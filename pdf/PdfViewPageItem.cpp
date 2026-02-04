#include "PdfViewPageItem.h"

#include <QPainter>
#include <QPdfDocument>

#include "PdfViewPageProvider.h"

PdfViewPageItem::PdfViewPageItem(const int number, PdfViewPageProvider* provider)
    : _provider(provider)
    , _number(number)
    , _pointSize(_provider->document()->pagePointSize(number))
    , _scaleCache(0.0)
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

    if (qFuzzyCompare(_scaleCache, scale)) {
        painter->drawImage(boundingRect(), _imageCache);
    }
    else
    {
        QFuture<PdfViewPageProvider::Response> response = _provider->enqueueRequest({ _number, scale });
        response.then([=, this](const PdfViewPageProvider::Response& value){
            if (const QImage* image = std::get_if<QImage>(&value)){
                _imageCache = *image;
                _scaleCache = scale;
                paint(painter, option, widget);
            }
        });
    }
}
