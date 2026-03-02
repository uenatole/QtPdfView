#pragma once

class QPointF;
class QPdfLink;

struct DocumentLinkSource
{
    virtual ~DocumentLinkSource() = default;

    virtual auto linkHit(int page, QPointF point) const -> bool = 0;
    virtual auto link(int page, QPointF point) const -> QPdfLink = 0;
};
