#include "PdfViewPageProvider.h"

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QPdfDocument>
#include <QScreen>
#include <QtConcurrent/QtConcurrentRun>

PdfViewPageProvider::PdfViewPageProvider()
{
    setCacheLimit(1 /*GiB*/ * 1024 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);
}

void PdfViewPageProvider::setDocument(QPdfDocument* document)
{
    _document = document;
}

void PdfViewPageProvider::setView(QGraphicsView* view)
{
    _view = view;
}

QPdfDocument* PdfViewPageProvider::document() const
{
    return _document;
}

void PdfViewPageProvider::setPixelRatio(const qreal ratio)
{
    // TODO: invalidate cache (?) or take ratio in account with {scale} (!)
    _pixelRatio = ratio;
}

void PdfViewPageProvider::setCacheLimit(const qreal bytes) const
{
    _cache.setLimit(bytes);
}

PdfViewPageProvider::RenderResponse PdfViewPageProvider::requestRender(const QGraphicsItem* requester, int page, qreal scale)
{
    if (const QImage* image = _cache.object(page, scale); image)
    {
        qDebug() << "Cache hit: page =" << page << "scale =" << scale;
        return RenderResponse { *image };
    }

    const RenderParameters parameters { page, scale, requester };
    const std::optional<QImage> nearestImage = findNearestImage(page, scale);

    // Check active render request for duplication
    if (_renderState)
    {
        if (_renderState->Parameters == parameters)
        {
            return RenderResponse { nearestImage };
        }

        if (_renderState->Parameters.Page == parameters.Page)
        {
            _renderState.reset();
        }
    }

    // Check pending render requests for duplication
    for (RenderRequest& request : _requests)
    {
        if (request.Parameters.Page == parameters.Page)
        {
            request.Parameters.Scale = parameters.Scale;
            return RenderResponse { nearestImage };
        }
    }

    // Enqueue new request
    const auto signal = enqueueRenderRequest(RenderRequest {parameters});

    // Dequeue it immediately to start render
    // TODO: add dequeue delay to prevent from 'zoom spamming' & etc
    if (!_renderState)
        tryDequeueRenderRequest();

    return RenderResponse {
        .NearestImage = nearestImage,
        .RenderTicket = signal
    };
}

PdfViewPageProvider::RenderCache::RenderCache()
{
    _storage.setOnEraseFn([this](const std::pair<int, qreal>& key)
    {
        _keySets[key.first].erase(key.second);
    });
}

QImage* PdfViewPageProvider::RenderCache::object(int page, qreal scale) const
{
    return _storage.object({ page, scale });
}

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

QImage* PdfViewPageProvider::RenderCache::nearestObject(int page, qreal targetScale) const
{
    const auto& scales = _keySets[page];
    const auto closestScaleIt = closest_element(scales, targetScale);

    if (closestScaleIt == scales.end())
        return nullptr;

    return _storage.object({ page, *closestScaleIt });
}

bool PdfViewPageProvider::RenderCache::insert(int page, qreal scale, QImage* image)
{
    if (const bool inserted = _storage.insert({ page, scale }, image, image->sizeInBytes()); Q_LIKELY(inserted))
    {
        _keySets[page].insert(scale);
        return true;
    }
    return false;
}

void PdfViewPageProvider::RenderCache::setLimit(std::size_t bytes)
{
    _storage.setMaxCost(bytes);
}

bool PdfViewPageProvider::isRequesterActual(const QGraphicsItem* item) const
{
    QRect portRect = _view->viewport()->rect();
    QRectF sceneRect = _view->mapToScene(portRect).boundingRect();
    QRectF itemRect = item->mapRectFromScene(sceneRect);

    return itemRect.intersects(item->boundingRect());
}

std::optional<QImage> PdfViewPageProvider::findNearestImage(int page, qreal scale)
{
    if (auto* image = _cache.nearestObject(page, scale); image)
        return *image;

    return std::nullopt;
}

QFuture<void> PdfViewPageProvider::enqueueRenderRequest(RenderRequest&& request)
{
    const RenderRequest& requestRef = _requests.emplace_back(std::move(request));

    auto chain0 = requestRef.Promise.future();
    auto chain1 = chain0.then(QThread::currentThread(), [this, parameters=requestRef.Parameters](const QImage& image){
        (void) _cache.insert(parameters.Page, parameters.Scale, new QImage(image));
        _renderState.reset();
        tryDequeueRenderRequest();
    });

    return chain1;
}

void PdfViewPageProvider::tryDequeueRenderRequest()
{
    // TODO: stop active render if it's not actual now

    // Erase unactual requests
    const auto firstActualIt = std::find_if(_requests.begin(), _requests.end(), [this](const RenderRequest& request)
    {
        const auto* requester = request.Parameters.Requester;
        return isRequesterActual(requester);
    });

    const auto prevSize = _requests.size();
    _requests.erase(_requests.begin(), firstActualIt);

    if (const auto diff = prevSize - _requests.size(); diff) qDebug() << "Erased" << diff << "elements";

    if (_requests.empty()) return;

    // Take first request in queue
    RenderRequest request = std::move(_requests.front());
    _requests.pop_front();

    QFuture<void> future = request.Promise.future();
    QThread* thread = QThread::create(
        [document=_document, page=request.Parameters.Page, scale=request.Parameters.Scale, ratio=_pixelRatio](QPromise<QImage> promise)
        {
            promise.start();

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

            promise.finish();
        }
        , std::move(request.Promise)
    );

    thread->start();
    _renderState.emplace(request.Parameters, future, thread);
}
