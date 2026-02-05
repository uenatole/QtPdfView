#include "PdfViewPageProvider.h"

#include <QElapsedTimer>
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
    const CacheKey key { page, scale };
    const RenderRequest request { page, scale };

    if (const QImage* image = _cache.object(key); image)
    {
        qDebug() << "Cache hit: page =" << key.first << "scale =" << key.second;
        return RenderResponses::Cached { *image };
    }

    // TODO: Do not drop the active job completely but cancel and move it to the second place in the queue.
    //       Item with cancelled render job will try to ::update() itself, and if item is visible,
    //       the request must be marked as "actual" to not be dropped completely.

    // Sometimes request may be sequentially duplicated, ignore it
    if (_activeRenderRequestOpt && request == *_activeRenderRequestOpt)
    {
        return RenderResponses::InProgress();
    }

    _activeRenderRequestJob.cancel();

    // Rendering is done in separate thread
    _activeRenderRequestOpt = request;
    _activeRenderRequestJob = QtConcurrent::run([document=_document, ratio=_pixelRatio, page, scale](QPromise<QImage>& promise)
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
        qDebug() << "Render finished: page =" << page << "scale =" << scale << " time =" << timer.elapsed() << "ms";

        promise.addResult(result);
    });

    // Caching is done in main thread
    const auto chain = _activeRenderRequestJob.then(QThread::currentThread(), [this, key](const QImage& image){
        _activeRenderRequestOpt = std::nullopt;
        const bool inserted = _cache.insert(key, new QImage(image), image.sizeInBytes());
        qDebug() << "Cache load: page =" << key.first << "scale=" << key.second << "inserted =" << inserted;
    });

    return RenderResponses::Scheduled { std::nullopt, chain };
}
