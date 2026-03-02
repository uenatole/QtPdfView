#pragma once

#include "core/DocumentLinkSource.h"
#include "core/DocumentTextSource.h"
#include "core/DocumentMetaSource.h"

class QPdfDocument;

class PdfDocumentParser : public DocumentTextSource, public DocumentLinkSource, public DocumentMetaSource
{
public:
    explicit PdfDocumentParser(const std::shared_ptr<QPdfDocument>& document);
    ~PdfDocumentParser() override;

    auto setLayoutCacheLimit(qreal bytes) const -> void;

    auto pageCount() const -> int override;
    auto pagePointSize(int page) const -> QSizeF override;

    auto textHit(int page, QPointF point, uint8_t lod) const -> bool final;
    auto textRegion() const -> std::unique_ptr<DocumentTextRegion> final;

    auto linkHit(int page, QPointF point) const -> bool final;
    auto link(int page, QPointF point) const -> QPdfLink final;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
