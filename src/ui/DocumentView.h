#pragma once

#include <QGraphicsView>
#include <QWidget>

#include "DocumentPageItem.h"

class Document;

// TODO: hide QGraphicsView
class DocumentView : public QGraphicsView
{
public:
    explicit DocumentView(QWidget* parent = nullptr);
    ~DocumentView() override;

    void setDocument(const std::shared_ptr<Document>& document);

    using QGraphicsView::transformationAnchor;
    using QGraphicsView::setTransformationAnchor;

    void setWheelZooming(bool enabled = true);
    bool wheelZooming() const;

    QString getSelectedText() const;

protected:
    void wheelEvent(QWheelEvent*) override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    friend struct RenderFeedback;
    friend struct PageItemFeedback;

    QGraphicsItem* getPageItem(int page) const;

    bool m_wheelZoomingDisabled = true;
    std::shared_ptr<Document> m_document;
    std::unique_ptr<DocumentPageItem::Feedback> m_feedback;

    std::optional<QPointF> m_selectionStart;
    QHash<int, QGraphicsItem*> m_pageItems;
};
