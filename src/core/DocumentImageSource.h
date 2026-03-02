#pragma once

#include <optional>
#include <QImage>

struct DocumentImageSource
{
    struct Feedback
    {
        virtual ~Feedback() = default;

        virtual bool isActual(int page) const = 0;
        virtual void imageReady(int page) const = 0;
    };

    virtual ~DocumentImageSource() = default;

    virtual auto requestImage(int page, qreal scale, Feedback* feedback) const -> std::optional<QImage> = 0;
};
