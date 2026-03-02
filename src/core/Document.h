#pragma once

#include <memory>
#include <QPdfLink>

#include "DocumentImageSource.h"
#include "DocumentMetaSource.h"

struct DocumentTextRegion;

struct DocumentImageSource;
struct DocumentTextSource;
struct DocumentLinkSource;
struct DocumentMetaSource;

class Document
{
public:
    Document();
    ~Document();

    auto setImageSource(const std::shared_ptr<DocumentImageSource>& imageSource) -> void;
    auto setTextSource(const std::shared_ptr<DocumentTextSource>& textSource) -> void;
    auto setLinkSource(const std::shared_ptr<DocumentLinkSource>& linkSource) -> void;
    auto setMetaSource(const std::shared_ptr<DocumentMetaSource>& metaSource) -> void;

    // TODO: ~~~
    auto setImageSourceFeedback(DocumentImageSource::Feedback* feedback) -> void;

    auto pageCount() const -> int;
    auto pageSize(int number) const -> QSizeF;

    auto requestImage(int number, qreal scale) const -> std::optional<QImage>;

    auto linkHit(int page, QPointF point) const -> bool;
    auto link(int page, QPointF point) const -> QPdfLink;

    auto textHit(int page, QPointF point, uint8_t lod = -1) const -> bool;
    auto textRegion() const -> std::unique_ptr<DocumentTextRegion>;

private:
    std::shared_ptr<DocumentImageSource> m_image;
    std::shared_ptr<DocumentTextSource> m_text;
    std::shared_ptr<DocumentLinkSource> m_link;
    std::shared_ptr<DocumentMetaSource> m_meta;

    DocumentImageSource::Feedback* m_imageFeedback;
};
