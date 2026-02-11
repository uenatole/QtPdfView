#include "PdfPageItem.h"

#include <QPdfDocument>

#include "PdfPageProvider.h"

PdfPageItem::PdfPageItem(const int number, PdfPageProvider* provider)
    : _provider(provider)
    , _number(number)
    , _pointSize(_provider->document()->pagePointSize(number))
{
    setCacheMode(NoCache);
    assert(number >= 0 && number < _provider->document()->pageCount());
}

QRectF PdfPageItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), _pointSize);
}

void PdfPageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    const qreal scale = painter->worldTransform().m11();

    painter->fillRect(boundingRect(), Qt::white);

    if (const auto image = _provider->request(reinterpret_cast<PdfPageProvider::Interface::RequesterID>(this), _number, scale); image)
        painter->drawImage(boundingRect(), *image);
}
