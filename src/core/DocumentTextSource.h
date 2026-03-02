#pragma once

#include <QRectF>

struct DocumentTextRegion
{
    virtual ~DocumentTextRegion() = default;

    virtual auto configure(int page, QRectF region, uint8_t lod = - 1) -> void = 0;

    virtual auto lod() const -> uint8_t = 0;
    virtual auto id() const -> uint64_t = 0;

    virtual auto text() const -> QString = 0;
    virtual auto geometry() const -> QList<QRectF> = 0;
};

struct DocumentTextSource
{
    virtual ~DocumentTextSource() = default;

    virtual auto textHit(int page, QPointF point, uint8_t lod = -1) const -> bool = 0;
    virtual auto textRegion() const -> std::unique_ptr<DocumentTextRegion> = 0;
};
