#pragma once

#include "core/DocumentRenderer.h"

class QPdfDocument;

class PdfDocumentRenderer : public DocumentRenderer
{
public:
    explicit PdfDocumentRenderer(const std::shared_ptr<QPdfDocument>& document);
    ~PdfDocumentRenderer() override;

    auto setPixelRatio(qreal ratio) const -> void;
    auto setRenderCacheLimit(qreal bytes) const -> void;
    auto setRenderDelay(int ms) const -> void;

    auto requestPageRender(int page, qreal scale, Feedback* feedback) const -> std::optional<QImage> final;

private:
    struct Private;
    const std::unique_ptr<Private> d;
};
