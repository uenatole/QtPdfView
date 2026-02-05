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

    struct RenderResponses
    {
        struct Cached
        {
            QImage Image;
        };

        struct Scheduled
        {
            std::optional<QImage> NearestImage;
            QFuture<void> Signal;
        };

        struct InProgress {};
    };

    using RenderResponse = std::variant<RenderResponses::Cached, RenderResponses::Scheduled, RenderResponses::InProgress>;
    RenderResponse requestRender(int page, qreal scale);

private:
    QPdfDocument* _document = nullptr;
    qreal _pixelRatio = 1.0;

    using CacheKey = std::pair<int, qreal>;
    mutable QCache<CacheKey, QImage> _cache;

    struct RenderRequest
    {
        int Page;
        qreal Scale;

        bool operator==(const RenderRequest& other) const
        {
            return Page == other.Page && qFuzzyCompare(Scale, other.Scale);
        }
    };

    mutable std::optional<RenderRequest> _activeRenderRequestOpt;
    mutable QFuture<QImage> _activeRenderRequestJob;
};
