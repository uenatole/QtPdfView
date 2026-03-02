#include "Document.h"

#include <QPageSize>
#include <QPdfLink>

#include "DocumentRenderer.h"
#include "DocumentParser.h"

namespace
{
    struct DummyParser : DocumentParser
    {
        auto pageCount() const -> int final { return 0; }
        auto pagePointSize(int) const -> QSizeF final { return {}; }
        auto textHit(int, QPointF, uint8_t) const -> bool final { return false; }

        auto textRegion() const -> std::unique_ptr<DocumentTextRegion> final
        {
            struct DummyTextRegion : DocumentTextRegion
            {
                auto configure(int, QRectF, uint8_t ) -> void final {}
                auto lod() const -> uint8_t final { return 0; }
                auto id() const -> uint64_t final { return 0; }
                auto text() const -> QString final { return {}; }
                auto geometry() const -> QList<QRectF> final { return {}; }
            };

            return std::make_unique<DummyTextRegion>();
        }

        auto linkHit(int, QPointF) const -> bool override { return false; }
        auto link(int, QPointF) const -> std::optional<DocumentLink> override { return std::nullopt; }
    };

    struct DummyRenderer : DocumentRenderer
    {
        auto requestPageRender(int page, qreal scale, DocumentRenderFeedback* feedback) const -> std::optional<QImage> override { return std::nullopt; }
    };
}

Document::Document()
    : m_parser(std::make_shared<DummyParser>())
    , m_renderer(std::make_shared<DummyRenderer>())
{}

Document::~Document() = default;

auto Document::setParser(const std::shared_ptr<DocumentParser>& parser) -> void
{
    m_parser = parser;
}

auto Document::setRenderer(const std::shared_ptr<DocumentRenderer>& renderer) -> void
{
    m_renderer = renderer;
}

auto Document::setImageSourceFeedback(DocumentRenderFeedback* feedback) -> void
{
    m_rendererFeedback = feedback;
}

auto Document::pageCount() const -> int
{
    return m_parser->pageCount();
}

auto Document::pageSize(int number) const -> QSizeF
{
    return m_parser->pagePointSize(number);
}

auto Document::requestImage(int number, qreal scale) const -> std::optional<QImage>
{
    return m_renderer->requestPageRender(number, scale, m_rendererFeedback);
}

auto Document::linkHit(int page, QPointF point) const -> bool
{
    return m_parser->linkHit(page, point);
}

auto Document::link(int page, QPointF point) const -> std::optional<DocumentLink>
{
    return m_parser->link(page, point);
}

auto Document::textHit(int page, QPointF point, uint8_t lod) const -> bool
{
    return m_parser->textHit(page, point, lod);
}

auto Document::textRegion() const -> std::unique_ptr<DocumentTextRegion>
{
    return m_parser->textRegion();
}
