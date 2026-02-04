#include "PdfViewPageProvider.h"

#include <QPdfDocument>

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

void PdfViewPageProvider::setCacheLimit(const qreal bytes) const
{
    _cache.setMaxCost(bytes);
}

QFuture<PdfViewPageProvider::Response> PdfViewPageProvider::enqueueRequest(const Request& request) const
{
    const CacheKey key { request.PageNumber, request.PageScaling };

    if (const QImage* image = _cache.object(key); image)
    {
        qDebug() << "Cache hit: page =" << key.first << "scale =" << key.second;
        return QtFuture::makeReadyValueFuture<Response>(*image);
    }

    const auto pointSize = _document->pagePointSize(request.PageNumber);
    const auto renderSize = pointSize * request.PageScaling * request.OutputPixelRatio;
    const auto image = _document->render(request.PageNumber, renderSize.toSize());

    _cache.insert(key, new QImage(image), image.sizeInBytes());
    return QtFuture::makeReadyValueFuture<Response>(image);
}
