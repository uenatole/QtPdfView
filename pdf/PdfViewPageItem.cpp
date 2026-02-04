#include "PdfViewPageItem.h"

#include <QPainter>
#include <QPdfDocument>

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
    Q_UNUSED(widget);

    const qreal scale = painter->worldTransform().m11();

    painter->fillRect(boundingRect(), Qt::white);

    const PdfViewPageProvider::Request request { _number, scale };
    QFuture<PdfViewPageProvider::Response> response = _provider->enqueueRequest(request);

    response.then([=, this](const PdfViewPageProvider::Response& v){
        if (const QImage* image = std::get_if<QImage>(&v); image) {
            painter->drawImage(boundingRect(), *image);
        }
    });
}
