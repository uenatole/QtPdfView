#pragma once

#include <QGraphicsView>
#include <QWidget>

#include <QPdfSelection>
#include "PdfPageItem.h"

class PdfPageProvider;
class QPdfDocument;

// NOTE: right now depends on QPdfSelection and used for convenience
// TODO: migrate from Qt::Pdf prior to own implementation
struct PdfViewSelection
{
    explicit PdfViewSelection(const QList<QPdfSelection>& selections);
    void copyToClipboard(QClipboard::Mode mode = QClipboard::Clipboard) const;

private:
    QList<QPdfSelection> m_selections;
};

// TODO: hide QGraphicsView
class PdfView : public QGraphicsView
{
public:
    explicit PdfView(QWidget* parent = nullptr);
    ~PdfView() override;

    void setDocument(QPdfDocument* document);

    using QGraphicsView::transformationAnchor;
    using QGraphicsView::setTransformationAnchor;

    void setWheelZooming(bool enabled = true);
    bool wheelZooming() const;

    PdfViewSelection getSelection() const;

    // TODO: hide it
    void processLink(const QPdfLink& link);

protected:
    void wheelEvent(QWheelEvent*) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool m_wheelZoomingDisabled = true;
    std::unique_ptr<PdfPageProvider> m_provider;
    std::unique_ptr<PdfPageItem::Feedback> m_feedback;

    std::optional<QPointF> m_selectionStart;
};
