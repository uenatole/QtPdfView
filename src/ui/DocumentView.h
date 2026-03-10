#pragma once

#include <QGraphicsView>
#include <QWidget>

#include "DocumentPageItem.h"

class DocumentFacade;

// TODO: hide QGraphicsView
class DocumentView : public QGraphicsView
{
public:
    explicit DocumentView(QWidget* parent = nullptr);
    ~DocumentView() override;

    void setDocument(const std::shared_ptr<DocumentFacade>& document);

    using QGraphicsView::transformationAnchor;
    using QGraphicsView::setTransformationAnchor;

    QString getSelectedText() const;

private:
    friend struct RenderFeedback;
    friend struct PageItemFeedback;

    QGraphicsItem* getPageItem(int page) const;

    std::shared_ptr<DocumentFacade> m_document;
    std::unique_ptr<DocumentPageItem::Feedback> m_feedback;

    std::optional<QPointF> m_selectionStart;
    QHash<int, QGraphicsItem*> m_pageItems;
};
