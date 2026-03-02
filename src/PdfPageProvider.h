#pragma once

#include <cstddef>
#include <optional>
#include <QImage>
#include <QtTypes>
#include <QPdfSelection> // TODO: hide under another abstraction
#include <QPdfLink>

#include "iface/DocumentImageSource.h"
#include "iface/DocumentLinkSource.h"
#include "iface/DocumentTextSource.h"

class QPdfDocument;

class PdfPageProvider : public DocumentImageSource, public DocumentTextSource, public DocumentLinkSource
{
public:
    PdfPageProvider();
    ~PdfPageProvider() override;

    void setDocument(QPdfDocument* document) const;

    void setFeedback(Feedback* interface) const;
    [[nodiscard]] Feedback* feedback() const;

    void setPixelRatio(qreal ratio) const;
    void setRenderCacheLimit(qreal bytes) const;
    void setRenderDelay(int ms) const;

    void setLayoutCacheLimit(qreal bytes) const;

    QSizeF pagePointSize(int page) const;

    auto linkHit(int page, QPointF point) const -> bool override;
    auto link(int page, QPointF point) const -> QPdfLink override;

    std::optional<QImage> requestImage(int page, qreal scale) final;

    bool textHit(int page, QPointF point, uint8_t lod = -1) const final;
    std::unique_ptr<DocumentTextRegion> textRegion() const final;

private:
    struct Private;
    std::unique_ptr<Private> d_ptr;
};
