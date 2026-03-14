#include "DocumentLink.h"

DocumentLink::Url::Url(const QUrl& url): m_url(url)
{}

auto DocumentLink::Url::url() const -> const QUrl&
{
    return m_url;
}

DocumentLink::Jump::Jump(const int destinationPage, const float destinationZoom, const QPointF destinationLocation)
    : m_destinationPage(destinationPage)
    , m_destinationZoom(destinationZoom)
    , m_destinationLocation(destinationLocation)
{}

auto DocumentLink::Jump::destinationPage() const -> int
{
    return m_destinationPage;
}

auto DocumentLink::Jump::destinationZoom() const -> float
{
    return m_destinationZoom;
}

auto DocumentLink::Jump::destinationLocation() const -> QPointF
{
    return m_destinationLocation;
}

DocumentLink::DocumentLink(const int page, const QList<QRectF>& geometry, const std::variant<Url, Jump>& contents): m_page(page)
    , m_geometry(geometry)
    , m_contents(contents)
{}

auto DocumentLink::page() const -> int
{
    return m_page;
}

auto DocumentLink::geometry() const -> const QList<QRectF>&
{
    return m_geometry;
}

auto DocumentLink::contents() const -> const Contents&
{
    return m_contents;
}

auto DocumentLink::toString() const -> QString
{
    QString contentStr = "?";

    if (const auto content = std::get_if<Url>(&m_contents); content)
    {
        contentStr = QString("Url { uri = %1 }")
            .arg(content->url().toString());
    }
    else if (const auto content = std::get_if<Jump>(&m_contents); content)
    {
        contentStr = QString("Jump { page = %1, point = (%2, %3), zoom = %4 }")
                     .arg(content->destinationPage())
                     .arg(content->destinationLocation().x())
                     .arg(content->destinationLocation().y())
                     .arg(content->destinationZoom());
    }

    return QString("DocumentLink { location { page = %1, geometry = (%2,%3 %4x%5) }, contents = %6 }")
           .arg(m_page)
           .arg(m_geometry[0].x())
           .arg(m_geometry[0].y())
           .arg(m_geometry[0].width())
           .arg(m_geometry[0].height())
           .arg(contentStr);
}
