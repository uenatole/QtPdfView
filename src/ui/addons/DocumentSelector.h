#pragma once

#include <QObject>
#include <QPoint>

class DocumentView;

class DocumentSelector : public QObject
{
public:
    explicit DocumentSelector(DocumentView* parent);

protected:
    bool eventFilter(QObject*, QEvent*) final;

private:
    void onPressed(QPoint);
    void onReleased(QPoint);
    void onMoved(QPoint) const;

    DocumentView* const m_view;

    std::optional<QPointF> m_start;
};
