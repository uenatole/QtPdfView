#pragma once

#include <QRectF>

struct DocumentTextSource
{
    virtual ~DocumentTextSource() = default;

    // TODO: replace QPolygonF with QRectF for simplicity
    virtual QList<QRectF> getGeometryAt(int page, QRectF region) = 0;
    virtual QString getTextAt(int page, QRectF region) = 0;
};
