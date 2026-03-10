#pragma once

#include <optional>
#include <QImage>

struct Document;

struct DocumentRenderFeedback
{
    virtual ~DocumentRenderFeedback() = default;

    virtual bool isActual(int page) const = 0;
    virtual void imageReady(int page) const = 0;
};

struct DocumentRenderer
{
    virtual ~DocumentRenderer() = default;

    virtual auto setDocument(std::shared_ptr<const Document> document) -> void = 0;

    virtual auto requestPageRender(int page, qreal scale, DocumentRenderFeedback* feedback) const -> std::optional<QImage> = 0;
};
