#pragma once

#include <QGraphicsItem>
#include <QPdfSelection>

class PdfPageProvider;

class PdfPageItem : public QGraphicsItem
{
public:
    PdfPageItem(PdfPageProvider* provider, int number);
    ~PdfPageItem() override;

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    void SetSelectionRect(const QRectF& rect);
    QPdfSelection GetSelection() const;

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    struct Private;
    std::unique_ptr<Private> d_ptr;
};
