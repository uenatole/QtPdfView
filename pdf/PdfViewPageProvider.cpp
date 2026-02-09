#include "PdfViewPageProvider.h"

#include <QElapsedTimer>
#include <QPdfDocument>
#include <QScreen>
#include <QtConcurrent/QtConcurrentRun>

PdfViewPageProvider::PdfViewPageProvider()
{
    setCacheLimit(1 /*GiB*/ * 1024 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);

    _cache.setOnEraseFn([this](const auto& key)
    {
        const auto& page = key.first;
        const auto& scale = key.second;
        _cacheKeyScales[page].erase(scale);
    });
}

void PdfViewPageProvider::setDocument(QPdfDocument* document)
{
    _document = document;
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
    _cache.setMaxCost(bytes);
}

PdfViewPageProvider::RenderResponse PdfViewPageProvider::requestRender(int page, qreal scale)
{
    const RenderParameters parameters { page, scale };

    if (const QImage* image = _cache.object(parameters); image)
    {
        qDebug() << "Cache hit: page =" << parameters.Page << "scale =" << parameters.Scale;
        return RenderResponse { *image };
    }

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

std::optional<QImage> PdfViewPageProvider::findNearestImage(int page, qreal scale)
{
    // TODO: find nearest by (page + scale)

    const auto& scales = _cacheKeyScales[page];
    const auto closestScaleIt = closest_element(scales, scale);

    if (closestScaleIt == scales.end())
        return std::nullopt;

    if (auto* image = _cache.object({ page, *closestScaleIt }); Q_LIKELY(image))
        return *image;

    return std::nullopt;
}

QFuture<void> PdfViewPageProvider::enqueueRenderRequest(RenderRequest&& request)
{
    const RenderRequest& requestRef = _requests.emplace_back(std::move(request));

    auto chain0 = requestRef.Promise.future();
    auto chain1 = chain0.then(QThread::currentThread(), [this, parameters=requestRef.Parameters](const QImage& image){
        const bool inserted = _cache.insert(parameters, new QImage(image), image.sizeInBytes());
        if (inserted)
            _cacheKeyScales[parameters.Page].insert(parameters.Scale);

        qDebug() << "Cache load: page =" << parameters.Page << "scale=" << parameters.Scale << "inserted =" << inserted;

        _renderState.reset();
        tryDequeueRenderRequest();
    });

    return chain1;
}

void PdfViewPageProvider::tryDequeueRenderRequest()
{
    // TODO: stop active render if it's not actual now
    // TODO: skip all unactual requests
    // NOTE: actuality is checked based on requester visibility. TODO: add way to check if requester is visible

    if (_renderState) return;
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
            qDebug() << "Render finished: page =" << page << "scale =" << scale << " time =" << timer.elapsed() << "ms";

            promise.addResult(result);

            promise.finish();
        }
        , std::move(request.Promise)
    );

    thread->start();
    _renderState.emplace(request.Parameters, future, thread);
}
