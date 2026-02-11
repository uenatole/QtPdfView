#pragma once

#include <QFuture>
#include <QGraphicsView>
#include <QTimer>

#include "custom/QCacheExt.h"

class QPdfDocument;

class PdfPageProvider
{
public:
    struct Interface
    {
        using RequesterID = std::size_t;

        virtual ~Interface() = default;
        [[nodiscard]] virtual bool isActual(RequesterID id) const = 0;
        virtual bool notify(RequesterID id) = 0;
    };

    PdfPageProvider();

    void setDocument(QPdfDocument* document);
    void setInterface(Interface* interface);

    QPdfDocument* document() const;

    void setPixelRatio(qreal ratio);
    void setCacheLimit(qreal bytes) const;
    void setRenderDelay(int ms);

    std::optional<QImage> request(Interface::RequesterID requester, int page, qreal scale);

private:
    class RenderCache
    {
    public:
        RenderCache();

        QImage* object(int page, qreal scale) const;
        QImage* nearestObject(int page, qreal targetScale) const;

        bool insert(int page, qreal scale, QImage* image);

        void setLimit(std::size_t bytes);

    private:
        mutable QCacheExt<std::pair<int, qreal>, QImage> _storage;
        mutable QHash<int, std::set<qreal>> _keySets;
    };

    struct RenderRequest
    {
        int Page;
        qreal Scale;
        Interface::RequesterID Requester;

        bool operator==(const RenderRequest& other) const
        {
            return Page == other.Page && qFuzzyCompare(Scale, other.Scale);
        }
    };

    struct RenderState {
        RenderRequest Request;
        QFuture<void> Future;

        RenderState(const RenderRequest& parameters, QFuture<void> future)
            : Request(parameters)
            , Future(std::move(future))
        {}

        ~RenderState()
        {
            Future.cancelChain();
        }
    };

    std::optional<QImage> findNearestImage(int page, qreal scale);

    void enqueueRenderRequest(RenderRequest&& request);
    void tryDequeueRenderRequestDelayed();
    void tryDequeueRenderRequest();

    Interface* _interface = nullptr;
    QPdfDocument* _document = nullptr;
    qreal _pixelRatio = 1.0;

    QTimer _dequeueDelayTimer;

    mutable RenderCache _cache;

    std::list<RenderRequest> _requests;
    std::optional<RenderState> _renderState;
};
