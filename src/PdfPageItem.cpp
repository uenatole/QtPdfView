#include "PdfPageItem.h"
#include "PdfPageItem.h"

#include <QPainter>
#include <QPdfDocument>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

#include "PdfPageProvider.h"

struct PdfPageItem::Private
{
    friend class PdfPageItem;

    Private(PdfPageProvider* provider, Feedback* feedback, const int number)
        : provider(provider)
        , feedback(feedback)
        , number(number)
        , pointSize(provider->pagePointSize(number))
    {}

private:
    PdfPageProvider* const provider;
    Feedback* const feedback;

    const int number;
    const QSizeF pointSize;

    QRectF selectionRect;
    QPdfLink currentLink;
};

PdfPageItem::PdfPageItem(PdfPageProvider* provider, Feedback* feedback, const int number)
    : d_ptr(new Private(provider, feedback, number))
{
    setCacheMode(NoCache);
    setAcceptHoverEvents(true);
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

    if (const auto image = d_ptr->provider->request(reinterpret_cast<PdfPageProvider::Feedback::RequesterID>(this), d_ptr->number, scale); image)
        painter->drawImage(boundingRect(), *image);

    if (const QRectF rect = d_ptr->selectionRect; !rect.isNull())
    {
        // TODO: add selection styling

        for (const QPdfSelection selection = GetSelection(); const QPolygonF& polygon : selection.bounds())
            painter->drawPolygon(polygon);
    }

    if (d_ptr->currentLink.isValid())
    {
        // TODO: add link highlight styling

        for (const QRectF rect : d_ptr->currentLink.rectangles())
            painter->drawRect(rect);

        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        unsetCursor();
    }
}

void PdfPageItem::SetSelectionRect(const QRectF& rect)
{
    d_ptr->selectionRect = rect;
    update();
}

QPdfSelection PdfPageItem::GetSelection() const
{
    const auto rect = d_ptr->selectionRect;
    return d_ptr->provider->getSelection(d_ptr->number, rect.topLeft(), rect.bottomRight());
}

int PdfPageItem::Number() const
{
    return d_ptr->number;
}

void PdfPageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    tryLinkHover(event->pos());
    QGraphicsItem::hoverMoveEvent(event);
}

void PdfPageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    d_ptr->currentLink = QPdfLink();
    update();
    QGraphicsItem::hoverLeaveEvent(event);
}

void PdfPageItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    tryLinkPress(event->pos());
    QGraphicsItem::mousePressEvent(event);
}

void PdfPageItem::tryLinkHover(const QPointF pos)
{
    const QPdfLink link = d_ptr->provider->getLinkAt(d_ptr->number, pos);

    const auto equals = [](const QPdfLink& first, const QPdfLink& second) -> bool {
        return first.rectangles() == second.rectangles(); // TODO: take overlapping links into account
    };

    if (equals(d_ptr->currentLink, link))
        return;

    d_ptr->currentLink = link;
    update();
}

void PdfPageItem::tryLinkPress(const QPointF pos)
{
    const QPdfLink link = d_ptr->provider->getLinkAt(d_ptr->number, pos);

    if (link.isValid())
        d_ptr->feedback->linkPressed(link);
}
