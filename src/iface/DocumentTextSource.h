#pragma once

#include <QRectF>

struct DocumentTextSource
{
    virtual ~DocumentTextSource() = default;

    // TODO: replace QPolygonF with QRectF for simplicity
    virtual QList<QRectF> getGeometryAt(int page, QPointF start, QPointF end) = 0;
    virtual QString getTextAt(int page, QPointF start, QPointF end) = 0;
};
