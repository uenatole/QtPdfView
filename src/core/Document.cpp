#include "Document.h"

#include <QPageSize>
#include <QPdfLink>

#include "DocumentImageSource.h"
#include "DocumentTextSource.h"
#include "DocumentLinkSource.h"
#include "DocumentMetaSource.h"

namespace
{
    struct ImageSourceDummy : DocumentImageSource
    {
        [[nodiscard]] auto requestImage(int page, qreal scale, Feedback* feedback) const -> std::optional<QImage> override
        {
            return std::nullopt;
        }
    };

    struct TextSourceDummy : DocumentTextSource
    {
        [[nodiscard]] auto textHit(int page, QPointF point, uint8_t lod) const -> bool override
        {
            return false;
        }

        [[nodiscard]] auto textRegion() const -> std::unique_ptr<DocumentTextRegion> override
        {
            struct TextRegionDummy : DocumentTextRegion
            {
                auto configure(int, QRectF, uint8_t) -> void final {}
                [[nodiscard]] auto lod() const -> uint8_t final { return 0; }
                [[nodiscard]] auto id() const -> uint64_t final { return 0;}
                [[nodiscard]] auto text() const -> QString final { return {}; }
                [[nodiscard]] auto geometry() const -> QList<QRectF> final { return {}; }
            };

            return std::make_unique<TextRegionDummy>();
        }
    };

    struct LinkSourceDummy : DocumentLinkSource
    {
        auto linkHit(int page, QPointF point) const -> bool override { return false; }
        auto link(int page, QPointF point) const -> QPdfLink override { return {}; }
    };

    struct MetaSourceDummy : DocumentMetaSource
    {
        auto pageCount() const -> int override { return 0; }
        auto pagePointSize(int page) const -> QSizeF override { return {}; }
    };
}

Document::Document()
    : m_image(std::make_shared<ImageSourceDummy>())
    , m_text(std::make_shared<TextSourceDummy>())
    , m_link(std::make_shared<LinkSourceDummy>())
    , m_meta(std::make_shared<MetaSourceDummy>())
    , m_imageFeedback(nullptr)
{}

Document::~Document() = default;

void Document::setImageSource(const std::shared_ptr<DocumentImageSource>& imageSource)
{
    m_image = imageSource;
}

void Document::setTextSource(const std::shared_ptr<DocumentTextSource>& textSource)
{
    m_text = textSource;
}

void Document::setLinkSource(const std::shared_ptr<DocumentLinkSource>& linkSource)
{
    m_link = linkSource;
}

auto Document::setMetaSource(const std::shared_ptr<DocumentMetaSource>& metaSource) -> void
{
    m_meta = metaSource;
}

auto Document::setImageSourceFeedback(DocumentImageSource::Feedback* feedback) -> void
{
    m_imageFeedback = feedback;
}

auto Document::pageCount() const -> int
{
    // TODO: impl
    return m_meta->pageCount();
}

auto Document::pageSize(int number) const -> QSizeF
{
    // TODO: impl
    return m_meta->pagePointSize(number);
}

auto Document::requestImage(int number, qreal scale) const -> std::optional<QImage>
{
    return m_image->requestImage(number, scale, m_imageFeedback);
}

auto Document::linkHit(int page, QPointF point) const -> bool
{
    return m_link->linkHit(page, point);
}

auto Document::link(int page, QPointF point) const -> QPdfLink
{
    return m_link->link(page, point);
}

auto Document::textHit(int page, QPointF point, uint8_t lod) const -> bool
{
    return m_text->textHit(page, point, lod);
}

auto Document::textRegion() const -> std::unique_ptr<DocumentTextRegion>
{
    return m_text->textRegion();
}
