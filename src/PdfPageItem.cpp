#include "PdfPageItem.h"
#include "PdfPageItem.h"

#include <QPainter>
#include <QPdfDocument>

#include "PdfPageProvider.h"

struct PdfPageItem::Private
{
    friend class PdfPageItem;

    Private(PdfPageProvider* provider, const int number)
        : provider(provider)
        , number(number)
        , pointSize(provider->pagePointSize(number))
    {}

private:
    PdfPageProvider* const provider;
    const int number;
    const QSizeF pointSize;

    QRectF selectionRect;
};

PdfPageItem::PdfPageItem(PdfPageProvider* provider, const int number)
    : d_ptr(new Private(provider, number))
{
    setCacheMode(NoCache);
    assert(number >= 0 && number < _provider->document()->pageCount());
}

PdfPageItem::~PdfPageItem() = default;

QRectF PdfPageItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), d_ptr->pointSize);
}

void PdfPageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    const qreal scale = painter->worldTransform().m11();

    painter->fillRect(boundingRect(), Qt::white);

    if (const auto image = d_ptr->provider->request(reinterpret_cast<PdfPageProvider::Interface::RequesterID>(this), d_ptr->number, scale); image)
        painter->drawImage(boundingRect(), *image);

    if (const QRectF rect = d_ptr->selectionRect; !rect.isNull())
    {
        // TODO: add selection styling

        const QPdfSelection selection = d_ptr->provider->getSelection(d_ptr->number, rect.topLeft(), rect.bottomRight());

        for (const QPolygonF& polygon : selection.bounds())
            painter->drawPolygon(polygon);
    }
}

void PdfPageItem::SetSelectionRect(const QRectF& rect)
{
    d_ptr->selectionRect = rect;
    update();
}

QRectF PdfPageItem::GetSelectionRect() const
{
    return d_ptr->selectionRect;
}
