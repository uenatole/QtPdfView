#pragma once

#include <QGraphicsItem>

class PdfPageProvider;

class PdfPageItem : public QGraphicsItem
{
public:
    PdfPageItem(int number, PdfPageProvider* provider);

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    PdfPageProvider* const _provider;

    const int _number;
    const QSizeF _pointSize;
};
