#pragma once

#include <QRectF>

struct DocumentTextSource
{
    virtual ~DocumentTextSource() = default;

    // TODO: replace QPolygonF with QRectF for simplicity
    virtual QList<QPolygonF> getGeometryAt(int page, QPointF start, QPointF end) = 0;
    virtual QString getTextAt(int page, QPointF start, QPointF end) = 0;
};
