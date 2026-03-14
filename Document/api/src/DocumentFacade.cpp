#include "DocumentFacade.h"

#include "Document.h"
#include "DocumentRenderer.h"
#include "DocumentParser.h"

namespace
{
    struct DummyParser : DocumentParser
    {
        auto setDocument(std::shared_ptr<const Document>) -> void final {}

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
        auto setDocument(std::shared_ptr<const Document>) -> void final {}

        auto requestPageRender(int page, qreal scale, DocumentRenderFeedback* feedback) const -> std::optional<QImage> override { return std::nullopt; }
    };
}

DocumentFacade::DocumentFacade()
    : m_parser(std::make_shared<DummyParser>())
    , m_renderer(std::make_shared<DummyRenderer>())
{}

DocumentFacade::~DocumentFacade() = default;

auto DocumentFacade::setDocument(const std::shared_ptr<Document>& document) -> void
{
    m_document = document;

    if (m_parser)
        m_parser->setDocument(document);

    if (m_renderer)
        m_renderer->setDocument(document);
}

auto DocumentFacade::setParser(const std::shared_ptr<DocumentParser>& parser) -> void
{
    m_parser = parser;
    m_parser->setDocument(m_document);
}

auto DocumentFacade::setRenderer(const std::shared_ptr<DocumentRenderer>& renderer) -> void
{
    m_renderer = renderer;
    m_renderer->setDocument(m_document);
}

auto DocumentFacade::setRenderFeedback(DocumentRenderFeedback* feedback) -> void
{
    m_rendererFeedback = feedback;
}

auto DocumentFacade::pageCount() const -> int
{
    return m_document->pageCount();
}

auto DocumentFacade::pageSize(int number) const -> QSizeF
{
    return m_document->pagePointSize(number);
}

auto DocumentFacade::requestImage(int number, qreal scale) const -> std::optional<QImage>
{
    return m_renderer->requestPageRender(number, scale, m_rendererFeedback);
}

auto DocumentFacade::linkHit(int page, QPointF point) const -> bool
{
    return m_parser->linkHit(page, point);
}

auto DocumentFacade::link(int page, QPointF point) const -> std::optional<DocumentLink>
{
    return m_parser->link(page, point);
}

auto DocumentFacade::textHit(int page, QPointF point, uint8_t lod) const -> bool
{
    return m_parser->textHit(page, point, lod);
}

auto DocumentFacade::textRegion() const -> std::unique_ptr<DocumentTextRegion>
{
    return m_parser->textRegion();
}
