#pragma once

#include <QImage>
#include <QFuture>

class QPdfDocument;

class PdfViewPageProvider
{
public:
    struct Request {
        int PageNumber;
        qreal PageScaling;
    };

    enum class Status { BadOptions, Cancelled };
    using Response = std::variant<QImage, Status>;

    void setDocument(QPdfDocument* document);
    QPdfDocument* document() const;

    QFuture<Response> enqueueRequest(const Request& request) const;

private:
    QPdfDocument* _document = nullptr;
};
