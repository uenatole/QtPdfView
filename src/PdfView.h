#pragma once

#include <QPdfSelection>
#include <QGraphicsView>
#include <QWidget>

#include "PdfPageItem.h"

class QPdfSelection;
class Document;

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

    void setDocument(const std::shared_ptr<Document>& document);

    using QGraphicsView::transformationAnchor;
    using QGraphicsView::setTransformationAnchor;

    void setWheelZooming(bool enabled = true);
    bool wheelZooming() const;

    QString getSelectedText() const;

    // TODO: hide it
    void processLink(const QPdfLink& link);

protected:
    void wheelEvent(QWheelEvent*) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    friend struct ImageSourceFeedback;
    QGraphicsItem* getPageItem(int page) const;

    bool m_wheelZoomingDisabled = true;
    std::shared_ptr<Document> m_document;
    std::unique_ptr<PdfPageItem::Feedback> m_feedback;

    std::optional<QPointF> m_selectionStart;
    QHash<int, QGraphicsItem*> m_pageItems;
};
