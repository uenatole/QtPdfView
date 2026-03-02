#pragma once

#include <QSizeF>

struct DocumentMetaSource
{
    virtual ~DocumentMetaSource() = default;

    virtual auto pageCount() const -> int = 0;
    virtual auto pagePointSize(int page) const -> QSizeF = 0;
};
