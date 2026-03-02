#include "PdfDocumentParser.h"

#include <QCache>
#include <QPdfLinkModel>

namespace
{
    using LineIndices = std::pair<int32_t, int32_t>;
    using CharIndices = std::pair<int32_t, int32_t>;

    struct LineLayout
    {
        CharIndices Indices; // [a; b)
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

struct PdfDocumentParser::Private
{
    explicit Private(const std::shared_ptr<QPdfDocument>& document_)
        : document(document_)
    {}

    QList<QRectF> getGeometry(const int page, const LineIndices& iLine, const CharIndices& iChar) const
    {
        const auto& [Lines, _] = getPageLayout(page);

        if (iLine.first == -1)
            return {};

        QList<QRectF> geometry;

        const auto firstLineIt = Lines.begin() + iLine.first;
        const auto lastLineIt = Lines.begin() + iLine.second;
        const auto startIndex = iChar.first;
        const auto endIndex = iChar.second;

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

    QString getText(const int page, const CharIndices& iChar) const
    {
        const auto startIndex = iChar.first;
        const auto endIndex = iChar.second;
        return document->getTextContentsAtIndex(page, startIndex, endIndex);
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
    auto getPageLayout(const int page) const -> const PageLayout&
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
                links.setDocument(document.get());
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

    auto getIndices(const int page, const QPointF start, const QPointF end) const -> std::pair<LineIndices, CharIndices>
    {
        const PageLayout& layout = getPageLayout(page);
        const auto [firstLineIt, lastLineIt] = layout.findLines(start, end);

        if (firstLineIt == layout.Lines.end())
        {
            return {{ -1, -1 }, { -1, -1 }};
        }

        return {
            {
                std::distance(layout.Lines.begin(), firstLineIt),
                std::distance(layout.Lines.begin(), lastLineIt),
            },
            {
                firstLineIt->getCharIndexAt(start.x()),
                lastLineIt->getCharIndexAt(end.x()) + 1,
            }
        };
    }

    friend class PdfDocumentParser;

    const std::shared_ptr<QPdfDocument> document;
    mutable QCache<int, PageLayout> pageLayoutCache;
};

PdfDocumentParser::PdfDocumentParser(const std::shared_ptr<QPdfDocument>& document)
    : d(new Private(document))
{
    setLayoutCacheLimit(64 /*MiB*/ * 1024 /*KiB*/ * 1024 /*B*/);
}

PdfDocumentParser::~PdfDocumentParser() = default;

auto PdfDocumentParser::setLayoutCacheLimit(qreal bytes) const -> void
{
    d->pageLayoutCache.setMaxCost(bytes);
}

auto PdfDocumentParser::pageCount() const -> int
{
    return d->document->pageCount();
}

auto PdfDocumentParser::pagePointSize(int page) const -> QSizeF
{
    return d->document->pagePointSize(page);
}

auto PdfDocumentParser::textHit(int page, QPointF point, uint8_t lod) const -> bool
{
    (void) lod; // TODO: hit test for different LoD
    return d->getIndices(page, point, point).first.first != -1;
}

auto PdfDocumentParser::textRegion() const -> std::unique_ptr<DocumentTextRegion>
{
    struct TextRegion : DocumentTextRegion
    {
        explicit TextRegion(Private* const d)
            : d_ptr(d)
        {}

        auto configure(const int page, const QRectF region, const uint8_t lod) -> void final
        {
            (void) lod;
            m_page = page;
            std::tie(m_iLine, m_iChar) = d_ptr->getIndices(page, region.topLeft(), region.bottomRight());
        }

        auto lod() const -> uint8_t final
        {
            return 0; // TODO: different Level-of-details support
        }

        auto id() const -> uint64_t final
        {
            return static_cast<int64_t>(m_iChar.first) << 32 | m_iChar.second;
        }

        auto text() const -> QString final
        {
            return d_ptr->getText(m_page, m_iChar);
        }

        auto geometry() const -> QList<QRectF> final
        {
            return d_ptr->getGeometry(m_page, m_iLine, m_iChar);
        }

    private:
        int m_page = -1;
        std::pair<int32_t, int32_t> m_iLine;
        std::pair<int32_t, int32_t> m_iChar;
        Private* const d_ptr;
    };

    return std::make_unique<TextRegion>(d.get());
}

auto PdfDocumentParser::linkHit(int page, QPointF point) const -> bool
{
    // TODO: possible optimization
    return d->getLink(page, point).isValid();
}

auto PdfDocumentParser::link(int page, QPointF point) const -> QPdfLink
{
    return d->getLink(page, point);
}
