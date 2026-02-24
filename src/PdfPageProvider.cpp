#include "PdfPageProvider.h"

#include <QCache>
#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QPdfDocument>
#include <QPdfLinkModel>
#include <QScreen>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>

#include "custom/QCacheExt.h"

namespace
{
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

    class RenderCache
    {
    public:
        RenderCache()
        {
            _storage.setOnEraseFn([this](const std::pair<int, qreal>& key)
            {
                _keySets[key.first].erase(key.second);
            });
        }

        QImage* object(int page, qreal scale) const
        {
            return _storage.object({ page, scale });
        }

        QImage* nearestObject(int page, const qreal targetScale) const
        {
            const auto& scales = _keySets[page];
            const auto closestScaleIt = closest_element(scales, targetScale);

            if (closestScaleIt == scales.end())
                return nullptr;

            return _storage.object({ page, *closestScaleIt });
        }

        bool insert(int page, qreal scale, QImage* image) const
        {
            if (const bool inserted = _storage.insert({ page, scale }, image, image->sizeInBytes()); Q_LIKELY(inserted))
            {
                _keySets[page].insert(scale);
                return true;
            }
            return false;
        }

        void setLimit(std::size_t bytes) const
        {
            _storage.setMaxCost(bytes);
        }

    private:
        mutable QCacheExt<std::pair<int, qreal>, QImage> _storage;
        mutable QHash<int, std::set<qreal>> _keySets;
    };

    struct RenderRequest
    {
        int Page;
        qreal Scale;

        bool operator==(const RenderRequest& other) const
        {
            return Page == other.Page && qFuzzyCompare(Scale, other.Scale);
        }
    };

    struct RenderState
    {
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

    struct LineLayout
    {
        QPair<int, int> Indices; // [a; b)
        QRectF Geometry;
        QList<qreal> CharBeginnings;

        QRectF getGeometryAtIndex(int begin, int end) const
        {
            if (begin >= Indices.second)
                return {};

            begin = std::max(Indices.first, begin);
            end = std::min(Indices.second, end);

            const qreal left = CharBeginnings[begin - Indices.first];
            const qreal right
                = end == Indices.second
                ? Geometry.right()
                : CharBeginnings[end - Indices.first];

            QRectF subGeometry = Geometry;
            subGeometry.setLeft(left);
            subGeometry.setRight(right);

            return subGeometry;
        }

        [[nodiscard]] int getCharIndexAt(qreal x) const
        {
            x = std::clamp(x, Geometry.left(), Geometry.right());

            const auto it = std::upper_bound(CharBeginnings.begin(), CharBeginnings.end(), x);
            const int offset = std::distance(CharBeginnings.begin(), it) - 1;

            return Indices.first + offset;
        }
    };

    struct PageLayout
    {
        QList<LineLayout> Lines;
        QList<QPdfLink> Links;

        [[nodiscard]] QPair<QList<LineLayout>::const_iterator, QList<LineLayout>::const_iterator> findLines(const QPointF begin, const QPointF end) const
        {
            const auto firstLineIt = std::lower_bound(Lines.begin(), Lines.end(), begin, [](const LineLayout& line, const QPointF& point) -> bool {
                return line.Geometry.bottom() < point.y();
            });

            const auto lastLineIt = std::upper_bound(Lines.begin(), Lines.end(), end, [](const QPointF& point, const LineLayout& line) -> bool {
                return line.Geometry.top() > point.y();
            });

            if (firstLineIt == lastLineIt)
                return { Lines.end(), Lines.end() };

            return {firstLineIt, lastLineIt - 1 };
        }
    };
}

struct PdfPageProvider::Private
{
    friend class PdfPageProvider;

    Private()
    {
        dequeueDelayTimer.setSingleShot(true);
        dequeueDelayTimer.setInterval(50);
        QObject::connect(&dequeueDelayTimer, &QTimer::timeout, [this]{ tryDequeueRenderRequest(); });
    }

    std::optional<QImage> request(const int page, const qreal scale)
    {
        if (const QImage* image = renderCache.object(page, scale); image)
        {
            qDebug() << "Cache hit: page =" << page << "scale =" << scale;
            return *image;
        }

        const RenderRequest parameters { page, scale };
        const std::optional<QImage> nearestImage = findNearestImage(page, scale);

        // Check active render request for duplication
        if (renderState)
        {
            if (renderState->Request == parameters)
            {
                return *nearestImage;
            }

            if (renderState->Request.Page == parameters.Page)
            {
                renderState.reset();
            }
        }

        // Check pending render requests for duplication
        for (RenderRequest& request : requests)
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
        if (!renderState)
            tryDequeueRenderRequestDelayed();

        return nearestImage;
    }

    QList<QRectF> getGeometry(const int page, const QPointF start, const QPointF end) const
    {
        const PageLayout& layout = getPageLayout(page);
        const auto [firstLineIt, lastLineIt] = layout.findLines(start, end);

        if (firstLineIt == layout.Lines.end())
            return {};

        QList<QRectF> geometry;

        const auto startIndex = firstLineIt->getCharIndexAt(start.x());
        const auto endIndex = lastLineIt->getCharIndexAt(end.x()) + 1;

        const QRectF firstLineGeometry = firstLineIt->getGeometryAtIndex(startIndex, endIndex);
        geometry.append(firstLineGeometry);

        if (firstLineIt != lastLineIt)
        {
            for (auto lineIt = firstLineIt + 1; lineIt < lastLineIt; ++lineIt)
                geometry.append(lineIt->Geometry);

            const QRectF lastLineGeometry = lastLineIt->getGeometryAtIndex(startIndex, endIndex);
            geometry.append(lastLineGeometry);
        }

        return geometry;
    }

    QString getText(const int page, const QPointF start, const QPointF end) const
    {
        const auto [regionStart, regionEnd] = getIndices(page, start, end);
        return document->getTextContentsAtIndex(page, regionStart, regionEnd);
    }

    QPdfLink getLink(const int page, const QPointF pos) const
    {
        const auto& layout = getPageLayout(page);

        for (const auto& link : layout.Links)
        {
            for (const auto& rect : link.rectangles())
                if (rect.contains(pos))
                    return link;
        }

        return {};
    }

private:
    std::optional<QImage> findNearestImage(const int page, const qreal scale) const
    {
        if (auto* image = renderCache.nearestObject(page, scale); image)
            return *image;

        return std::nullopt;
    }

    void enqueueRenderRequest(RenderRequest&& request)
    {
        requests.emplace_back(std::move(request));
    }

    void tryDequeueRenderRequestDelayed()
    {
        dequeueDelayTimer.start();
    }

    void tryDequeueRenderRequest()
    {
        if (renderState && !interface->isActual(renderState->Request.Page))
        {
            renderState.reset();
        }

        // Erase unactual requests
        const auto firstActualIt = std::find_if(requests.begin(), requests.end(), [this](const RenderRequest& request)
        {
            return interface->isActual(request.Page);
        });

        const auto prevSize = requests.size();
        requests.erase(requests.begin(), firstActualIt);

        if (const auto diff = prevSize - requests.size(); diff) qDebug() << "Erased" << diff << "elements";

        if (requests.empty()) return;

        // Take first request in queue
        RenderRequest request = std::move(requests.front());
        requests.pop_front();

        QFuture<void> future = QtConcurrent::run(
            [document=document, page=request.Page, scale=request.Scale, ratio=pixelRatio](QPromise<QImage>& promise)
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
            (void) renderCache.insert(request.Page, request.Scale, new QImage(image));
            interface->imageReady(request.Page);

            renderState.reset();
            tryDequeueRenderRequest();
        });

        renderState.emplace(request, future);
    }

    const PageLayout& getPageLayout(const int page) const
    {
        PageLayout* layout = pageLayoutCache.object(page);

        if (!layout)
        {
            layout = new PageLayout();

            // Line forming method
            {
                QList<QRectF> charBoxes = document->getCharGeometryAtIndex(page);
                QList<LineLayout> lines;

                // TODO: change line detection method because right now it sometimes is wrong
                const auto isOnLine = [](const QRectF& charRect, const QRectF& lineRect) -> bool
                {
                    return charRect.top() <= lineRect.bottom() && charRect.bottom() >= lineRect.top();
                };

                LineLayout currentLine = {};
                int startIndex = 0;

                for (int i = 0; i < charBoxes.size(); ++i)
                {
                    const auto& box = charBoxes[i];

                    if (currentLine.CharBeginnings.isEmpty())
                    {
                        currentLine.Geometry = box;
                        currentLine.CharBeginnings.append(box.left());
                        startIndex = i;
                    }
                    else
                    {
                        if (isOnLine(currentLine.Geometry, box))
                        {
                            currentLine.Geometry |= box; // TODO: optimize calculations
                            currentLine.CharBeginnings.append(box.left());
                        }
                        else
                        {
                            currentLine.Indices = qMakePair(startIndex, i);
                            lines.append(currentLine);

                            currentLine = {};

                            currentLine.Geometry = box;
                            currentLine.CharBeginnings.append(box.left());
                            startIndex = i;
                        }
                    }
                }

                if (!currentLine.CharBeginnings.isEmpty())
                {
                    currentLine.Indices = qMakePair(startIndex, charBoxes.size());
                    lines.append(currentLine);
                }

                layout->Lines = lines;
            }

            (void) pageLayoutCache.insert(page, layout); // TODO: make it work calculating layout size after creation

            // TODO: get rid off QPdfLinkModel
            {
                QPdfLinkModel links;
                links.setDocument(document);
                links.setPage(page);

                for (int i = 0; i < links.rowCount(QModelIndex()); ++i)
                {
                    QVariant variant = links.data(links.index(i), static_cast<int>(QPdfLinkModel::Role::Link));
                    QPdfLink link = variant.value<QPdfLink>();
                    layout->Links.append(link);
                }
            }

            qDebug() << "Layout" << page;
            for (const auto& line : layout->Lines)
                qDebug() << "    " << line.Indices << line.Geometry << line.Geometry.top();
        }

        return *layout;
    }

    QPair<int, int> getIndices(const int page, const QPointF start, const QPointF end) const
    {
        const PageLayout& layout = getPageLayout(page);
        const auto [firstLineIt, lastLineIt] = layout.findLines(start, end);

        if (firstLineIt == layout.Lines.end())
        {
            return { -1, -1 };
        }

        const auto startIndex = firstLineIt->getCharIndexAt(start.x());
        const auto endIndex = lastLineIt->getCharIndexAt(end.x()) + 1;

        return { startIndex, endIndex };
    }

    Feedback* interface = nullptr;
    QPdfDocument* document = nullptr;
    qreal pixelRatio = 1.0;

    QTimer dequeueDelayTimer;

    mutable RenderCache renderCache;

    std::list<RenderRequest> requests;
    std::optional<RenderState> renderState;

    mutable QCache<int, PageLayout> pageLayoutCache;
};

PdfPageProvider::PdfPageProvider()
    : d_ptr(new Private)
{
    setRenderCacheLimit(512 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);
    setLayoutCacheLimit(64 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);
}

PdfPageProvider::~PdfPageProvider() = default;

void PdfPageProvider::setDocument(QPdfDocument* document) const
{
    d_ptr->document = document;
}

void PdfPageProvider::setFeedback(Feedback* interface) const
{
    d_ptr->interface = interface;
}

PdfPageProvider::Feedback* PdfPageProvider::feedback() const
{
    return d_ptr->interface;
}

void PdfPageProvider::setPixelRatio(const qreal ratio) const
{
    // TODO: invalidate cache (?) or take ratio in account with {scale} (!)
    d_ptr->pixelRatio = ratio;
}

void PdfPageProvider::setRenderCacheLimit(const qreal bytes) const
{
    d_ptr->renderCache.setLimit(bytes);
}

void PdfPageProvider::setRenderDelay(const int ms) const
{
    d_ptr->dequeueDelayTimer.setInterval(ms);
}

void PdfPageProvider::setLayoutCacheLimit(qreal bytes) const
{
    d_ptr->pageLayoutCache.setMaxCost(bytes);
}

QSizeF PdfPageProvider::pagePointSize(int page) const
{
    return d_ptr->document->pagePointSize(page);
}

QPdfLink PdfPageProvider::getLinkAt(int page, QPointF pos) const
{
    return d_ptr->getLink(page, pos);
}

std::optional<QImage> PdfPageProvider::requestImage(const int page, const qreal scale)
{
    return d_ptr->request(page, scale);
}

QList<QRectF> PdfPageProvider::getGeometryAt(int page, QPointF start, QPointF end)
{
    return d_ptr->getGeometry(page, start, end);
}

QString PdfPageProvider::getTextAt(int page, QPointF start, QPointF end)
{
    return d_ptr->getText(page, start, end);
}

QPair<int, int> PdfPageProvider::getIndicesAt(int page, QPointF start, QPointF end)
{
    return d_ptr->getIndices(page, start, end);
}
