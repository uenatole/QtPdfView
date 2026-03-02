#pragma once

#include <QGraphicsItem>

class Document;
class QPdfLink;

class PdfPageItem : public QGraphicsItem
{
public:
    struct Feedback
    {
        virtual ~Feedback() = default;
        virtual void linkPressed(const QPdfLink&) = 0;
    };

    PdfPageItem(const std::shared_ptr<Document>& document, Feedback* feedback, int number);
    ~PdfPageItem() override;

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void SetSelectionRect(const QRectF& rect);
    QString GetSelectedText() const;

    int Number() const;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;

private:
    void updateCurrentLink(const QPdfLink& link);
    void updateCursorShape(std::optional<QPointF> pos = std::nullopt);

    struct Private;
    std::unique_ptr<Private> d_ptr;
};
