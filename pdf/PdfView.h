#pragma once

#include <QGraphicsView>
#include <QWidget>

class PdfViewPageProvider;
class QPdfDocument;

class PdfView : QGraphicsView
{
public:
    explicit PdfView(QWidget* parent = nullptr);
    ~PdfView() override;

    using QGraphicsView::show;

    void setDocument(QPdfDocument* document);

    using QGraphicsView::transformationAnchor;
    using QGraphicsView::setTransformationAnchor;

    void setWheelZooming(bool enabled = true);
    bool wheelZooming() const;

protected:
    void wheelEvent(QWheelEvent*) override;

private:
    bool m_wheelZoomingDisabled = true;
    std::unique_ptr<PdfViewPageProvider> m_provider;
};
