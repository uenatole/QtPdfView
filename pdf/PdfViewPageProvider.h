#pragma once

#include <QImage>
#include <QFuture>
#include <QCache>

class QPdfDocument;

class PdfViewPageProvider
{
public:
    struct Request {
        int PageNumber;
        qreal PageScaling;
        qreal OutputPixelRatio;
    };

    enum class Status { BadOptions, Cancelled };
    using Response = std::variant<QImage, Status>;

    PdfViewPageProvider();

    void setDocument(QPdfDocument* document);
    QPdfDocument* document() const;

    void setCacheLimit(qreal bytes) const;

    QFuture<Response> enqueueRequest(const Request& request) const;

private:
    QPdfDocument* _document = nullptr;

    using CacheKey = std::pair<int, qreal>;
    mutable QCache<CacheKey, QImage> _cache;
};
