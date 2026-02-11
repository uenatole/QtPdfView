#include "PdfPageProvider.h"

#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QPdfDocument>
#include <QScreen>
#include <QtConcurrent/QtConcurrentRun>

PdfPageProvider::PdfPageProvider()
{
    setCacheLimit(1 /*GiB*/ * 1024 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);

    _dequeueDelayTimer.setSingleShot(true);
    _dequeueDelayTimer.setInterval(50);
    QObject::connect(&_dequeueDelayTimer, &QTimer::timeout, [this]{ tryDequeueRenderRequest(); });
}

void PdfPageProvider::setDocument(QPdfDocument* document)
{
    _document = document;
}

void PdfPageProvider::setInterface(Interface* interface)
{
    _interface = interface;
}

QPdfDocument* PdfPageProvider::document() const
{
    return _document;
}

void PdfPageProvider::setPixelRatio(const qreal ratio)
{
    // TODO: invalidate cache (?) or take ratio in account with {scale} (!)
    _pixelRatio = ratio;
}

void PdfPageProvider::setCacheLimit(const qreal bytes) const
{
    _cache.setLimit(bytes);
}

void PdfPageProvider::setRenderDelay(int ms)
{
    _dequeueDelayTimer.setInterval(ms);
}

std::optional<QImage> PdfPageProvider::request(Interface::RequesterID requester, int page, qreal scale)
{
    if (const QImage* image = _cache.object(page, scale); image)
    {
        qDebug() << "Cache hit: page =" << page << "scale =" << scale;
        return *image;
    }

    const RenderRequest parameters { page, scale, requester };
    const std::optional<QImage> nearestImage = findNearestImage(page, scale);

    // Check active render request for duplication
    if (_renderState)
    {
        if (_renderState->Request == parameters)
        {
            return *nearestImage;
        }

        if (_renderState->Request.Page == parameters.Page)
        {
            _renderState.reset();
        }
    }

    // Check pending render requests for duplication
    for (RenderRequest& request : _requests)
    {
        if (request.Page == parameters.Page)
        {
            request.Scale = parameters.Scale;
            return *nearestImage;
        }
    }

    // Enqueue new request
    enqueueRenderRequest(RenderRequest {parameters});

    // Dequeue it delayed to start render and let for some requests to be outdated
    if (!_renderState)
        tryDequeueRenderRequestDelayed();

    return nearestImage;
}

PdfPageProvider::RenderCache::RenderCache()
{
    _storage.setOnEraseFn([this](const std::pair<int, qreal>& key)
    {
        _keySets[key.first].erase(key.second);
    });
}

QImage* PdfPageProvider::RenderCache::object(int page, qreal scale) const
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

QImage* PdfPageProvider::RenderCache::nearestObject(int page, qreal targetScale) const
{
    const auto& scales = _keySets[page];
    const auto closestScaleIt = closest_element(scales, targetScale);

    if (closestScaleIt == scales.end())
        return nullptr;

    return _storage.object({ page, *closestScaleIt });
}

bool PdfPageProvider::RenderCache::insert(int page, qreal scale, QImage* image)
{
    if (const bool inserted = _storage.insert({ page, scale }, image, image->sizeInBytes()); Q_LIKELY(inserted))
    {
        _keySets[page].insert(scale);
        return true;
    }
    return false;
}

void PdfPageProvider::RenderCache::setLimit(std::size_t bytes)
{
    _storage.setMaxCost(bytes);
}

std::optional<QImage> PdfPageProvider::findNearestImage(int page, qreal scale)
{
    if (auto* image = _cache.nearestObject(page, scale); image)
        return *image;

    return std::nullopt;
}

void PdfPageProvider::enqueueRenderRequest(RenderRequest&& request)
{
    _requests.emplace_back(std::move(request));
}

void PdfPageProvider::tryDequeueRenderRequestDelayed()
{
    _dequeueDelayTimer.start();
}

void PdfPageProvider::tryDequeueRenderRequest()
{
    if (_renderState && !_interface->isActual(_renderState->Request.Requester))
    {
        _renderState.reset();
    }

    // Erase unactual requests
    const auto firstActualIt = std::find_if(_requests.begin(), _requests.end(), [this](const RenderRequest& request)
    {
        return _interface->isActual(request.Requester);
    });

    const auto prevSize = _requests.size();
    _requests.erase(_requests.begin(), firstActualIt);

    if (const auto diff = prevSize - _requests.size(); diff) qDebug() << "Erased" << diff << "elements";

    if (_requests.empty()) return;

    // Take first request in queue
    RenderRequest request = std::move(_requests.front());
    _requests.pop_front();

    QFuture<void> future = QtConcurrent::run(
        [document=_document, page=request.Page, scale=request.Scale, ratio=_pixelRatio](QPromise<QImage>& promise)
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
        (void) _cache.insert(request.Page, request.Scale, new QImage(image));
        _interface->notify(request.Requester);

        _renderState.reset();
        tryDequeueRenderRequest();
    });

    _renderState.emplace(request, future);
}
