#pragma once

#include <optional>
#include <QImage>

struct DocumentImageSource
{
    struct Feedback
    {
        virtual ~Feedback() = default;

        virtual bool isActual(int page) = 0;
        virtual void imageReady(int page) = 0;
    };

    virtual ~DocumentImageSource() = default;
    [[nodiscard]] virtual std::optional<QImage> requestImage(int page, qreal scale) = 0;
};
