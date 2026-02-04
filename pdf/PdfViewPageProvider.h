#pragma once

#include <QImage>
#include <QFuture>
#include <QCache>

class QPdfDocument;

class PdfViewPageProvider
{
public:
    PdfViewPageProvider();

    void setDocument(QPdfDocument* document);
    QPdfDocument* document() const;

    void setPixelRatio(qreal ratio);
    void setCacheLimit(qreal bytes) const;

    enum class ResponseAsyncStatus { BadOptions, Cancelled };
    using ResponseAsync = std::variant<QImage, ResponseAsyncStatus>;

    QFuture<ResponseAsync> getRenderAsync(int page, qreal scale) const;

private:
    QPdfDocument* _document = nullptr;
    qreal _pixelRatio = 1.0;

    using CacheKey = std::pair<int, qreal>;
    mutable QCache<CacheKey, QImage> _cache;
};
