#pragma once

#include <optional>
#include <QImage>

struct DocumentRenderer
{
    struct Feedback
    {
        virtual ~Feedback() = default;

        virtual bool isActual(int page) const = 0;
        virtual void imageReady(int page) const = 0;
    };

    virtual ~DocumentRenderer() = default;

    virtual auto requestPageRender(int page, qreal scale, Feedback* feedback) const -> std::optional<QImage> = 0;
};
