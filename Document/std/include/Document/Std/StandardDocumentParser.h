#pragma once

#include <Document/API/DocumentParser.h>

class StandardDocumentParser : public DocumentParser
{
public:
    StandardDocumentParser();
    ~StandardDocumentParser() override;

    auto setLayoutCacheLimit(qreal bytes) const -> void;

    auto setDocument(std::shared_ptr<const Document> document) -> void final;

    auto textHit(int page, QPointF point, uint8_t lod) const -> bool final;
    auto textRegion() const -> std::unique_ptr<DocumentTextRegion> final;

    auto linkHit(int page, QPointF point) const -> bool final;
    auto link(int page, QPointF point) const -> std::optional<DocumentLink> final;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
