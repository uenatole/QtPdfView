#pragma once

#include <QRectF>
#include <QUrl>

struct DocumentLink
{
    struct Url
    {
        explicit Url(const QUrl& url);

        auto url() const -> const QUrl&;

    private:
        QUrl m_url;
    };

    struct Jump
    {
        Jump(int destinationPage, float destinationZoom, QPointF destinationLocation);

        auto destinationPage() const -> int;
        auto destinationZoom() const -> float;
        auto destinationLocation() const -> QPointF;

    private:
        int m_destinationPage;
        float m_destinationZoom;
        QPointF m_destinationLocation;
    };

    using Contents = std::variant<Url, Jump>;

    DocumentLink(int page, const QList<QRectF>& geometry, const std::variant<Url, Jump>& contents);

    auto page() const -> int;
    auto geometry() const -> const QList<QRectF>&;
    auto contents() const -> const Contents&;
    auto toString() const -> QString;

private:
    int m_page;
    QList<QRectF> m_geometry;
    std::variant<Url, Jump> m_contents;
};
