#pragma once

#include <cstddef>
#include <optional>
#include <QImage>
#include <QtTypes>
#include <QPdfSelection> // TODO: hide under another abstraction
#include <QPdfLink>

#include "iface/DocumentImageSource.h"

class QPdfDocument;

class PdfPageProvider : public DocumentImageSource
{
public:
    PdfPageProvider();
    ~PdfPageProvider() override;

    void setDocument(QPdfDocument* document) const;

    void setFeedback(Feedback* interface) const;
    [[nodiscard]] Feedback* feedback() const;

    void setPixelRatio(qreal ratio) const;
    void setCacheLimit(qreal bytes) const;
    void setRenderDelay(int ms) const;

    QSizeF pagePointSize(int page) const;
    QPdfSelection getSelection(int page, QPointF start = {}, QPointF end = {}) const;

    QPdfLink getLinkAt(int page, QPointF pos) const;

    std::optional<QImage> requestImage(int page, qreal scale) final;

private:
    struct Private;
    std::unique_ptr<Private> d_ptr;
};
