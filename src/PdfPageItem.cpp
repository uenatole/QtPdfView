#include "PdfPageItem.h"

#include <QPainter>
#include <QPdfDocument>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

#include "core/Document.h"
#include "core/DocumentParser.h"

struct PdfPageItem::Private
{
    friend class PdfPageItem;

    Private(const std::shared_ptr<Document>& document, Feedback* feedback, const int number)
        : document(document)
        , feedback(feedback)
        , textRegion(document->textRegion())
        , number(number)
        , pointSize(document->pageSize(number))
    {}

private:
    std::shared_ptr<Document> const document;
    Feedback* const feedback;
    const std::unique_ptr<DocumentTextRegion> textRegion;

    const int number;
    const QSizeF pointSize;

    QRectF selectionRect;
    QPdfLink currentLink;
};

PdfPageItem::PdfPageItem(const std::shared_ptr<Document>& document, Feedback* feedback, const int number)
    : d_ptr(new Private(document, feedback, number))
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

    if (const auto image = d_ptr->document->requestImage(d_ptr->number, scale); image)
        painter->drawImage(boundingRect(), *image);

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Multiply);

    if (const QRectF rect = d_ptr->selectionRect; !rect.isNull())
    {
        // TODO: make text selection style configurable

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(206, 235, 249, 200));

        for (const QList<QRectF> geometries = d_ptr->textRegion->geometry(); const QRectF& geometry : geometries)
            painter->drawRect(geometry.adjusted(-0, -2, +0, +2));
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
        d_ptr->selectionRect = rect;

        const auto idTmp = d_ptr->textRegion->id();
        d_ptr->textRegion->configure(d_ptr->number, rect);

        if (idTmp != d_ptr->textRegion->id())
        {
            update();
        }
    }
}

QString PdfPageItem::GetSelectedText() const
{
    return d_ptr->textRegion->text();
}

int PdfPageItem::Number() const
{
    return d_ptr->number;
}

void PdfPageItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    updateCurrentLink(d_ptr->document->link(d_ptr->number, event->pos()));
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
    updateCurrentLink(d_ptr->document->link(d_ptr->number, event->pos()));
    updateCursorShape(event->pos());
    QGraphicsItem::mouseMoveEvent(event);
}

void PdfPageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (const QPdfLink link = d_ptr->document->link(d_ptr->number, event->pos()); link.isValid())
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
    else if (d_ptr->document->textHit(d_ptr->number, *pos))
    {
        setCursor(Qt::CursorShape::IBeamCursor);
    }
    else
    {
        unsetCursor();
    }
}
