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

    QPair<int, int> selectionIndices = { -1, -1 };
    QRectF selectionRect;
    QPdfLink currentLink;
};

PdfPageItem::PdfPageItem(PdfPageProvider* provider, Feedback* feedback, const int number)
    : d_ptr(new Private(provider, feedback, number))
{
    setCacheMode(NoCache);
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable, true);
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

    // TODO: draw as underlay after other operations to exclude possible composition interference (~~~)
    painter->fillRect(boundingRect(), Qt::white);

    if (const auto image = d_ptr->provider->requestImage(d_ptr->number, scale); image)
        painter->drawImage(boundingRect(), *image);

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Multiply);

    if (const QRectF rect = d_ptr->selectionRect; !rect.isNull())
    {
        // TODO: make text selection style configurable

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(206, 235, 249, 200));

        // NOTE: right now QPolygonF is guaranteed to be QRectF, so draw it boundaries to easily add margins
        for (const QList<QPolygonF> polygons = d_ptr->provider->getGeometryAt(d_ptr->number, d_ptr->selectionRect.topLeft(), d_ptr->selectionRect.bottomRight()); const QPolygonF& polygon : polygons)
            painter->drawRect(polygon.boundingRect().adjusted(-0, -2, +0, +2));
    }

    if (d_ptr->currentLink.isValid())
    {
        // TODO: make link highlighting style configurable

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 204, 160));

        for (const QRectF rect : d_ptr->currentLink.rectangles())
            painter->drawRect(rect.adjusted(-0, -2, +0, +2));
    }

    painter->restore();
}

void PdfPageItem::SetSelectionRect(const QRectF& rect)
{
    if (rect != d_ptr->selectionRect)
    {
        // NOTE: it's optimized way to compare "selections"
        // TODO: aggregate 'indices', 'geometry', 'text' into one interface instance "DocumentTextSelection"
        const auto indices = rect.isNull()
            ? QPair { -1, -1 }
            : d_ptr->provider->getIndicesAt(d_ptr->number, rect.topLeft(), rect.bottomRight());

        d_ptr->selectionRect = rect;

        if (indices != d_ptr->selectionIndices)
        {
            d_ptr->selectionIndices = indices;
            update();
        }
    }
}

QString PdfPageItem::GetSelectedText() const
{
    const auto rect = d_ptr->selectionRect;
    return d_ptr->provider->getTextAt(d_ptr->number, rect.topLeft(), rect.bottomRight());
}

int PdfPageItem::Number() const
{
    return d_ptr->number;
}

void PdfPageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    updateCurrentLink(d_ptr->provider->getLinkAt(d_ptr->number, event->pos()));
    updateCursorShape(event->pos());
    QGraphicsItem::hoverMoveEvent(event);
}

void PdfPageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    updateCurrentLink(QPdfLink());
    updateCursorShape();
    QGraphicsItem::hoverLeaveEvent(event);
}

void PdfPageItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    updateCurrentLink(d_ptr->provider->getLinkAt(d_ptr->number, event->pos()));
    updateCursorShape(event->pos());
    QGraphicsItem::mouseMoveEvent(event);
}

void PdfPageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (const QPdfLink link = d_ptr->provider->getLinkAt(d_ptr->number, event->pos()); link.isValid())
        d_ptr->feedback->linkPressed(link);

    updateCursorShape(event->pos());
    QGraphicsItem::mouseReleaseEvent(event);
}

void PdfPageItem::updateCurrentLink(const QPdfLink& link)
{
    const auto equals = [](const QPdfLink& first, const QPdfLink& second) -> bool {
        return first.rectangles() == second.rectangles(); // TODO: take overlapping links into account
    };

    if (equals(d_ptr->currentLink, link))
        return;

    d_ptr->currentLink = link;
    update();
}

void PdfPageItem::updateCursorShape(std::optional<QPointF> pos)
{
    if (!pos)
    {
        unsetCursor();
    }
    else if (d_ptr->currentLink.isValid())
    {
        setCursor(Qt::CursorShape::PointingHandCursor);
    }
    else if (const auto geom = d_ptr->provider->getGeometryAt(d_ptr->number, *pos, *pos); !geom.isEmpty())
    {
        setCursor(Qt::CursorShape::IBeamCursor);
    }
    else
    {
        unsetCursor();
    }
}
