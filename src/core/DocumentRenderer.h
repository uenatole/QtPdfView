#pragma once

#include <optional>
#include <QImage>

struct DocumentRenderFeedback
{
    virtual ~DocumentRenderFeedback() = default;

    virtual bool isActual(int page) const = 0;
    virtual void imageReady(int page) const = 0;
};

struct DocumentRenderer
{
    virtual ~DocumentRenderer() = default;

    virtual auto requestPageRender(int page, qreal scale, DocumentRenderFeedback* feedback) const -> std::optional<QImage> = 0;
};
