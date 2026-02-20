#pragma once

#include <cstddef>
#include <optional>
#include <QImage>
#include <QtTypes>
#include <QPdfSelection> // TODO: hide under another abstraction
#include <QPdfLink>

class QPdfDocument;

class PdfPageProvider
{
public:
    struct Interface
    {
        using RequesterID = std::size_t;

        virtual ~Interface() = default;
        [[nodiscard]] virtual bool isActual(RequesterID id) const = 0;
        virtual bool notify(RequesterID id) = 0;
    };

    PdfPageProvider();
    ~PdfPageProvider();

    void setDocument(QPdfDocument* document) const;

    void setInterface(Interface* interface) const;
    Interface* interface() const;

    void setPixelRatio(qreal ratio) const;
    void setCacheLimit(qreal bytes) const;
    void setRenderDelay(int ms) const;

    QSizeF pagePointSize(int page) const;
    QPdfSelection getSelection(int page, QPointF start = {}, QPointF end = {}) const;

    QPdfLink getLinkAt(int page, QPointF pos) const;

    std::optional<QImage> request(Interface::RequesterID requester, int page, qreal scale) const;

private:
    struct Private;
    std::unique_ptr<Private> d_ptr;
};
