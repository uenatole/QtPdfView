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

void PdfViewPageProvider::setPixelRatio(const qreal ratio)
{
    // TODO: invalidate cache (?) or take ratio in account with {scale} (!)
    _pixelRatio = ratio;
}

void PdfViewPageProvider::setCacheLimit(const qreal bytes) const
{
    _cache.setMaxCost(bytes);
}

QFuture<PdfViewPageProvider::ResponseAsync> PdfViewPageProvider::getRenderAsync(int page, qreal scale) const
{
    const CacheKey key { page, scale };

    if (const QImage* image = _cache.object(key); image)
    {
        qDebug() << "Cache hit: page =" << key.first << "scale =" << key.second;
        return QtFuture::makeReadyValueFuture<ResponseAsync>(*image);
    }

    const auto pointSize = _document->pagePointSize(page);
    const auto renderSize = pointSize * scale * _pixelRatio;
    const auto image = _document->render(page, renderSize.toSize());

    _cache.insert(key, new QImage(image), image.sizeInBytes());
    return QtFuture::makeReadyValueFuture<ResponseAsync>(image);
}
