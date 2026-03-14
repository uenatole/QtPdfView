#pragma once

#include <Document/API/DocumentRenderer.h>

class StandardDocumentRenderer : public DocumentRenderer
{
public:
    StandardDocumentRenderer();
    ~StandardDocumentRenderer() override;

    auto setPixelRatio(qreal ratio) const -> void;
    auto setRenderCacheLimit(qreal bytes) const -> void;
    auto setRenderDelay(int ms) const -> void;

    auto setDocument(std::shared_ptr<const Document> document) -> void final;

    auto requestPageRender(int page, qreal scale, DocumentRenderFeedback* feedback) const -> std::optional<QImage> final;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
