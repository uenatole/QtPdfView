#pragma once

#include <QGraphicsItem>
#include <QPdfSelection>

class PdfPageProvider;
class QPdfLink;

class PdfPageItem : public QGraphicsItem
{
public:
    struct Feedback
    {
        virtual ~Feedback() = default;
        virtual void linkPressed(const QPdfLink&) = 0;
    };

    PdfPageItem(PdfPageProvider* provider, Feedback* feedback, int number);
    ~PdfPageItem() override;

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void SetSelectionRect(const QRectF& rect);
    QPdfSelection GetSelection() const;

    int Number() const;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

private:
    void tryLinkHover(QPointF pos);
    void tryLinkPress(QPointF pos);

    struct Private;
    std::unique_ptr<Private> d_ptr;
};
