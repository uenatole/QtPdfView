#include "PdfDocumentRenderer.h"

#include <QPdfDocument>
#include <QtConcurrent/QtConcurrentRun>

#include <QTimer>
#include <QElapsedTimer>

#include "custom/QCacheExt.h"

namespace
{
    template<typename Set>
    auto closest_element(Set& set, const typename Set::value_type& value)
        -> decltype(set.begin())
    {
        const auto it = set.lower_bound(value);
        if (it == set.begin())
            return it;

        const auto prev_it = std::prev(it);
        return (it == set.end() || value - *prev_it <= *it - value) ? prev_it : it;
    }

    class RenderCache
    {
    public:
        RenderCache()
        {
            _storage.setOnEraseFn([this](const std::pair<int, qreal>& key)
            {
                _keySets[key.first].erase(key.second);
            });
        }

        QImage* object(int page, qreal scale) const
        {
            return _storage.object({ page, scale });
        }

        QImage* nearestObject(int page, const qreal targetScale) const
        {
            const auto& scales = _keySets[page];
            const auto closestScaleIt = closest_element(scales, targetScale);

            if (closestScaleIt == scales.end())
                return nullptr;

            return _storage.object({ page, *closestScaleIt });
        }

        bool insert(int page, qreal scale, QImage* image) const
        {
            if (const bool inserted = _storage.insert({ page, scale }, image, image->sizeInBytes()); Q_LIKELY(inserted))
            {
                _keySets[page].insert(scale);
                return true;
            }
            return false;
        }

        void setLimit(std::size_t bytes) const
        {
            _storage.setMaxCost(bytes);
        }

    private:
        mutable QCacheExt<std::pair<int, qreal>, QImage> _storage;
        mutable QHash<int, std::set<qreal>> _keySets;
    };

    struct RenderRequest
    {
        int Page;
        qreal Scale;
        DocumentImageSource::Feedback* Feedback {};

        bool operator==(const RenderRequest& other) const
        {
            return Page == other.Page && qFuzzyCompare(Scale, other.Scale);
        }
    };

    struct RenderState
    {
        RenderRequest Request;
        QFuture<void> Future;

        RenderState(const RenderRequest& parameters, QFuture<void> future)
            : Request(parameters)
            , Future(std::move(future))
        {}

        ~RenderState()
        {
            Future.cancelChain();
        }
    };
}

struct PdfDocumentRenderer::Private
{
    explicit Private(const std::shared_ptr<QPdfDocument>& document_)
        : document(document_)
    {
        dequeueDelayTimer.setSingleShot(true);
        dequeueDelayTimer.setInterval(50);
        QObject::connect(&dequeueDelayTimer, &QTimer::timeout, [this]{ tryDequeueRenderRequest(); });
    }

    std::optional<QImage> request(const int page, const qreal scale, Feedback* feedback)
    {
        if (const QImage* image = renderCache.object(page, scale); image)
        {
            qDebug() << "Cache hit: page =" << page << "scale =" << scale;
            return *image;
        }

        RenderRequest request { page, scale, feedback };
        const std::optional<QImage> nearestImage = findNearestImage(page, scale);

        // Check active render request for duplication
        if (renderState)
        {
            if (renderState->Request == request)
            {
                return *nearestImage;
            }

            if (renderState->Request.Page == request.Page)
            {
                renderState.reset();
            }
        }

        // Check pending render requests for duplication
        for (RenderRequest& other : requests)
        {
            if (other.Page == request.Page)
            {
                other.Scale = request.Scale;
                return *nearestImage;
            }
        }

        // Enqueue new request
        enqueueRenderRequest(std::move(request));

        // Dequeue it delayed to start render and let for some requests to be outdated
        if (!renderState)
            tryDequeueRenderRequestDelayed();

        return nearestImage;
    }

private:
    std::optional<QImage> findNearestImage(const int page, const qreal scale) const
    {
        if (auto* image = renderCache.nearestObject(page, scale); image)
            return *image;

        return std::nullopt;
    }

    void enqueueRenderRequest(RenderRequest&& request)
    {
        requests.emplace_back(std::move(request));
    }

    void tryDequeueRenderRequestDelayed()
    {
        dequeueDelayTimer.start();
    }

    void tryDequeueRenderRequest()
    {
        if (renderState && !renderState->Request.Feedback->isActual(renderState->Request.Page))
        {
            renderState.reset();
        }

        // Erase unactual requests
        const auto firstActualIt = std::find_if(requests.begin(), requests.end(), [](const RenderRequest& request)
        {
            return request.Feedback->isActual(request.Page);
        });

        const auto prevSize = requests.size();
        requests.erase(requests.begin(), firstActualIt);

        if (const auto diff = prevSize - requests.size(); diff) qDebug() << "Erased" << diff << "elements";

        if (requests.empty()) return;

        // Take first request in queue
        RenderRequest request = std::move(requests.front());
        requests.pop_front();

        QFuture<void> future = QtConcurrent::run(
            [document=document, page=request.Page, scale=request.Scale, ratio=pixelRatio](QPromise<QImage>& promise)
            {
                struct PromiseCancel : QPdfDocument::ICancel {
                    explicit PromiseCancel(QPromise<QImage>& promise) : m_promise(promise) {}
                    bool isCancelled() final
                    {
                        return m_promise.isCanceled();
                    }

                private:
                    QPromise<QImage>& m_promise;
                };

                const auto pointSize = document->pagePointSize(page);
                const auto renderSize = pointSize * scale * ratio;
                const auto size = renderSize.toSize();

                const auto cancel = std::make_unique<PromiseCancel>(promise);

                QElapsedTimer timer;
                timer.start();
                const QImage result = document->render2(page, size, cancel.get());

                if (!result.isNull())
                    qDebug() << "Render finished: page =" << page << "scale =" << scale << " time =" << timer.elapsed() << "ms";

                promise.addResult(result);
            }
        )
        .then(QThread::currentThread(), [this, request](const QImage& image){
            (void) renderCache.insert(request.Page, request.Scale, new QImage(image));
            request.Feedback->imageReady(request.Page);

            renderState.reset();
            tryDequeueRenderRequest();
        });

        renderState.emplace(request, future);
    }

    friend class PdfDocumentRenderer;

    const std::shared_ptr<QPdfDocument> document;

    qreal pixelRatio = 1.0;

    QTimer dequeueDelayTimer;

    mutable RenderCache renderCache;

    std::list<RenderRequest> requests;
    std::optional<RenderState> renderState;
};

PdfDocumentRenderer::PdfDocumentRenderer(const std::shared_ptr<QPdfDocument>& document)
    : d(new Private(document))
{
    setRenderCacheLimit(512 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);
}

PdfDocumentRenderer::~PdfDocumentRenderer() = default;

auto PdfDocumentRenderer::setPixelRatio(qreal ratio) const -> void
{
    // TODO: invalidate cache (?) or take ratio in account with {scale} (!)
    d->pixelRatio = ratio;
}

auto PdfDocumentRenderer::setRenderCacheLimit(qreal bytes) const -> void
{
    d->renderCache.setLimit(bytes);
}

auto PdfDocumentRenderer::setRenderDelay(int ms) const -> void
{
    d->dequeueDelayTimer.setInterval(ms);
}

auto PdfDocumentRenderer::requestImage(int page, qreal scale, Feedback* feedback) const -> std::optional<QImage>
{
    return d->request(page, scale, feedback);
}
