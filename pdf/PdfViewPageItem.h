#pragma once

#include <QGraphicsItem>

class PdfViewPageProvider;

class PdfViewPageItem : public QGraphicsItem
{
public:
    PdfViewPageItem(int number, PdfViewPageProvider* provider);

    QRectF boundingRect() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    PdfViewPageProvider* const _provider;

    const int _number;
    const QSizeF _pointSize;
};
