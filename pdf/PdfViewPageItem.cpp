#include "PdfViewPageItem.h"

#include <QPainter>
#include <QPdfDocument>
#include <QWidget>

#include "PdfViewPageProvider.h"

PdfViewPageItem::PdfViewPageItem(const int number, PdfViewPageProvider* provider)
    : _provider(provider)
    , _number(number)
    , _pointSize(_provider->document()->pagePointSize(number))
{
    assert(number >= 0 && number < _provider->document()->pageCount());
}

QRectF PdfViewPageItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), _pointSize);
}

void PdfViewPageItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    const qreal scale = painter->worldTransform().m11();

    painter->fillRect(boundingRect(), Qt::white);

    QFuture<PdfViewPageProvider::ResponseAsync> response = _provider->getRenderAsync(_number, scale);
    response.then([=, this](const PdfViewPageProvider::ResponseAsync& v){
        if (const QImage* image = std::get_if<QImage>(&v); image) {
            painter->drawImage(boundingRect(), *image);
        }
    });
}
