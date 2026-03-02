#pragma once

#include <QRectF>
#include <QPdfLink>

struct DocumentTextRegion
{
    virtual ~DocumentTextRegion() = default;

    virtual auto configure(int page, QRectF region, uint8_t lod = - 1) -> void = 0;

    virtual auto lod() const -> uint8_t = 0;
    virtual auto id() const -> uint64_t = 0;

    virtual auto text() const -> QString = 0;
    virtual auto geometry() const -> QList<QRectF> = 0;
};

struct DocumentParser
{
    virtual ~DocumentParser() = default;

    virtual auto pageCount() const -> int = 0;
    virtual auto pagePointSize(int page) const -> QSizeF = 0;

    virtual auto textHit(int page, QPointF point, uint8_t lod = -1) const -> bool = 0;
    virtual auto textRegion() const -> std::unique_ptr<DocumentTextRegion> = 0;

    // TODO: get rid off QPdfLink to remove pdf-specific term
    virtual auto linkHit(int page, QPointF point) const -> bool = 0;
    virtual auto link(int page, QPointF point) const -> QPdfLink = 0;
};
