#pragma once

#include <QImage>
#include <QFuture>
#include "../custom/QCacheExt.h"

class QPdfDocument;

class PdfViewPageProvider
{
public:
    PdfViewPageProvider();

    void setDocument(QPdfDocument* document);
    QPdfDocument* document() const;

    void setPixelRatio(qreal ratio);
    void setCacheLimit(qreal bytes) const;

    struct RenderResponse
    {
        std::optional<QImage> NearestImage;
        std::optional<QFuture<void>> RenderTicket;
    };

    RenderResponse requestRender(int page, qreal scale);

private:
    using CacheKey = std::pair<int, qreal>;

    struct RenderParameters
    {
        int Page;
        qreal Scale;

        bool operator==(const RenderParameters& other) const
        {
            return Page == other.Page && qFuzzyCompare(Scale, other.Scale);
        }

        operator CacheKey() const {
            return { Page, Scale };
        }
    };

    struct RenderRequest
    {
        RenderParameters Parameters {};
        QPromise<QImage> Promise;

        explicit RenderRequest(const RenderParameters& parameters)
            : Parameters(parameters)
        {}

        RenderRequest(RenderRequest&& other) noexcept
        {
            Parameters = other.Parameters;
            Promise = std::move(other.Promise);
        }
    };

    struct RenderState {
        RenderParameters Parameters;
        QFuture<void> Future;
        std::unique_ptr<QThread> Thread;

        RenderState(const RenderParameters& parameters, QFuture<void> future, QThread* thread)
            : Parameters(parameters)
            , Future(std::move(future))
            , Thread(thread)
        {}

        ~RenderState()
        {
            Future.cancel();
            Thread->quit();
        }
    };

    std::optional<QImage> findNearestImage(int page, qreal scale);

    QFuture<void> enqueueRenderRequest(RenderRequest&& request);
    void tryDequeueRenderRequest();

    QPdfDocument* _document = nullptr;
    qreal _pixelRatio = 1.0;

    mutable QCacheExt<CacheKey, QImage> _cache;
    mutable QHash<int, std::set<qreal>> _cacheKeyScales;

    std::list<RenderRequest> _requests;
    std::optional<RenderState> _renderState;
};
