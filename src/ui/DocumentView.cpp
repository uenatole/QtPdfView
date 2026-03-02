#include "DocumentView.h"

#include <QDesktopServices>
#include <QGraphicsScene>
#include <QGraphicsEffect>
#include <QWheelEvent>

#include "core/Document.h"
#include "core/DocumentRenderer.h"

struct ImageSourceFeedback : DocumentRenderFeedback
{
    explicit ImageSourceFeedback(DocumentView* view) : _view(view){}

    [[nodiscard]] bool isActual(const int page) const final
    {
        const auto item = _view->getPageItem(page);

        const QRect portRect = _view->viewport()->rect();
        const QRectF sceneRect = _view->mapToScene(portRect).boundingRect();
        const QRectF itemRect = item->mapRectFromScene(sceneRect);

        return itemRect.intersects(item->boundingRect());
    }

    void imageReady(const int page) const final
    {
        const auto item = _view->getPageItem(page);
        item->update();
    }

private:
    DocumentView* _view;
};

struct PageItemFeedback : DocumentPageItem::Feedback
{
    explicit PageItemFeedback(DocumentView* view) : _view(view){}

    void linkPressed(const DocumentLink& link) override
    {
        if (const auto contents = std::get_if<DocumentLink::Url>(&link.contents()); contents)
        {
            QDesktopServices::openUrl(contents->url());
        }

        if (const auto contents = std::get_if<DocumentLink::Jump>(&link.contents()); contents)
        {
            const auto number = contents->destinationPage();
            const auto location = contents->destinationLocation();
            const auto zoom = contents->destinationZoom();

            QTransform t = _view->transform();
            t.setMatrix(
                zoom, t.m12(), t.m13(),
                t.m21(), zoom, t.m23(),
                t.m31(), t.m32(), t.m33()
            );

            const auto page = dynamic_cast<DocumentPageItem*>(_view->getPageItem(number));
            _view->setTransform(t);
            page->ensureVisible({ location, location });
        }
    }

private:
    DocumentView* const _view;
};

DocumentView::DocumentView(QWidget* parent)
    : QGraphicsView(parent)
    , m_feedback(new PageItemFeedback(this))
{}

DocumentView::~DocumentView(){}

void DocumentView::setDocument(const std::shared_ptr<Document>& document)
{
    m_document = document;
    m_document->setImageSourceFeedback(new ImageSourceFeedback(this));

    auto* scene = new QGraphicsScene();
    scene->setBackgroundBrush(palette().brush(QPalette::Dark));

    constexpr auto documentMargins = 6;

    qreal yCursor = documentMargins;
    qreal maxPageWidth = std::numeric_limits<qreal>::min();

    for (int page = 0; page < document->pageCount(); ++page)
    {
        QSizeF pagePointSize = document->pageSize(page);

        const auto item = new DocumentPageItem(document, m_feedback.get(), page);
        item->setPos(documentMargins, yCursor);

        yCursor += pagePointSize.height() + documentMargins;
        maxPageWidth = std::max(maxPageWidth, pagePointSize.width());

        // NOTE: According to the performance profiler, this causes large lags when scaling large
        // const auto shadowEffect = new QGraphicsDropShadowEffect();
        // shadowEffect->setBlurRadius(10.0);
        // shadowEffect->setColor(QColor(0, 0, 0, 150));
        // shadowEffect->setOffset(2, 2);
        // item->setGraphicsEffect(shadowEffect);

        scene->addItem(item);
        m_pageItems.insert(page, item);
    }

    setScene(scene);
    setSceneRect(0, 0, maxPageWidth + 2 * documentMargins, yCursor);

    centerOn(0, 0);
    setTransformationAnchor(AnchorUnderMouse);
}

void DocumentView::setWheelZooming(bool enabled)
{
    m_wheelZoomingDisabled = !enabled;
}

bool DocumentView::wheelZooming() const
{
    return m_wheelZoomingDisabled;
}

QString DocumentView::getSelectedText() const
{
    QString text;

    for (const QGraphicsItem* page : m_pageItems.asKeyValueRange() | std::views::values)
        text += dynamic_cast<const DocumentPageItem*>(page)->GetSelectedText();

    return text;
}

void DocumentView::wheelEvent(QWheelEvent* event)
{
    if (m_wheelZoomingDisabled)
    {
        QGraphicsView::wheelEvent(event);
        return;
    }

    constexpr auto calculateZoomStep = [](const qreal currentZoomFactor, const int sign) -> qreal
    {
        constexpr qreal baseStep = 0.1;
        constexpr qreal ceilStep = 0.9;

        const auto factor = std::ceil(currentZoomFactor + sign * baseStep);
        return qMin(baseStep * factor, ceilStep);
    };

    if (event->modifiers() & Qt::ControlModifier)
    {
        const int delta = event->angleDelta().y();

        const ViewportAnchor anchor = transformationAnchor();
        setTransformationAnchor(AnchorUnderMouse);

        const qreal currentZoomFactor = transform().m11();
        const qreal stepSize = calculateZoomStep(currentZoomFactor, (delta > 0) ? +1 : -1);

        const qreal zoomChange = (delta > 0) ? stepSize : -stepSize;
        const qreal newZoomFactor = qBound(0.1, currentZoomFactor + zoomChange, 10.0);

        const auto ff = newZoomFactor / currentZoomFactor;

        scale(ff, ff);
        setTransformationAnchor(anchor);

        event->accept();
    }
    else
    {
        QGraphicsView::wheelEvent(event);
    }
}

void DocumentView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        for (const auto item : items())
            if (const auto page = dynamic_cast<DocumentPageItem*>(item); page)
                page->SetSelectionRect({});

        m_selectionStart = mapToScene(event->pos());
    }

    QGraphicsView::mousePressEvent(event);
}

void DocumentView::mouseReleaseEvent(QMouseEvent* event)
{
    m_selectionStart.reset();

    QGraphicsView::mouseReleaseEvent(event);
}

void DocumentView::mouseMoveEvent(QMouseEvent* event)
{
    // NOTE: this progressive selection method is quite inefficient due to selection from the very beginning on every update.
    // TODO: provide stateful selection API to make it truly progressive.
    if (m_selectionStart)
    {
        const auto first = *m_selectionStart;
        const auto second = mapToScene(event->pos());
        const auto selectionRect = QRectF(first, second);

        for (const auto item : items())
            if (const auto page = dynamic_cast<DocumentPageItem*>(item); page)
            {
                const QRectF sceneIntersectionRect = selectionRect.intersected(page->sceneBoundingRect());

                if (sceneIntersectionRect.isNull())
                    continue;

                const QRectF pageIntersectionRect = page->mapRectFromScene(sceneIntersectionRect);
                page->SetSelectionRect(pageIntersectionRect);
            }
    }

    QGraphicsView::mouseMoveEvent(event);
}

QGraphicsItem* DocumentView::getPageItem(const int page) const
{
    return m_pageItems[page];
}


