#pragma once

#include <memory>

#include "DocumentLink.h"
#include "DocumentRenderer.h"

struct DocumentRenderer;
struct DocumentParser;
struct DocumentTextRegion;

class Document
{
public:
    Document();
    ~Document();

    auto setParser(const std::shared_ptr<DocumentParser>& parser) -> void;
    auto setRenderer(const std::shared_ptr<DocumentRenderer>& renderer) -> void;

    // TODO: ~~~
    auto setImageSourceFeedback(DocumentRenderer::Feedback* feedback) -> void;

    auto pageCount() const -> int;
    auto pageSize(int number) const -> QSizeF;

    auto requestImage(int number, qreal scale) const -> std::optional<QImage>;

    auto linkHit(int page, QPointF point) const -> bool;
    auto link(int page, QPointF point) const -> std::optional<DocumentLink>;

    auto textHit(int page, QPointF point, uint8_t lod = -1) const -> bool;
    auto textRegion() const -> std::unique_ptr<DocumentTextRegion>;

private:
    std::shared_ptr<DocumentParser> m_parser;
    std::shared_ptr<DocumentRenderer> m_renderer;
    DocumentRenderer::Feedback* m_rendererFeedback = nullptr;
};
