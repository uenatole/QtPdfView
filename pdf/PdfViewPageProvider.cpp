#include "PdfViewPageProvider.h"

#include <QPdfDocument>

void PdfViewPageProvider::setDocument(QPdfDocument* document)
{
    _document = document;
}

QPdfDocument* PdfViewPageProvider::document() const
{
    return _document;
}

QFuture<PdfViewPageProvider::Response> PdfViewPageProvider::enqueueRequest(const Request& request) const
{
    const auto pointSize = _document->pagePointSize(request.PageNumber);
    const auto renderSize = pointSize * request.PageScaling;
    const auto image = _document->render(request.PageNumber, renderSize.toSize());

    return QtFuture::makeReadyValueFuture<Response>(image);
}
