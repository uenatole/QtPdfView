#include "PdfView.h"

#include <QDesktopServices>
#include <QPdfDocument>
#include <QGraphicsScene>
#include <QGraphicsEffect>
#include <QWheelEvent>

#include "PdfPageItem.h"
#include "PdfPageProvider.h"

PdfViewSelection::PdfViewSelection(const QList<QPdfSelection>& selections)
    : m_selections(selections)
{}

void PdfViewSelection::copyToClipboard(const QClipboard::Mode mode) const
{
    QString text;
    for (const auto& selection : m_selections)
        text += selection.text();

    QGuiApplication::clipboard()->setText(text, mode);
}

struct ImageSourceFeedback : PdfPageProvider::Feedback
{
    explicit ImageSourceFeedback(PdfView* view) : _view(view){}

    [[nodiscard]] bool isActual(const int page) override
    {
        const auto item = _view->getPageItem(page);

        const QRect portRect = _view->viewport()->rect();
        const QRectF sceneRect = _view->mapToScene(portRect).boundingRect();
        const QRectF itemRect = item->mapRectFromScene(sceneRect);

        return itemRect.intersects(item->boundingRect());
    }

    void imageReady(const int page) override
    {
        const auto item = _view->getPageItem(page);
        item->update();
    }

private:
    PdfView* _view;
};

struct PageItemFeedback : PdfPageItem::Feedback
{
    explicit PageItemFeedback(PdfView* view) : _view(view){}

    void linkPressed(const QPdfLink& link) override
    {
        _view->processLink(link);
    }

private:
    PdfView* const _view;
};

PdfView::PdfView(QWidget* parent)
    : QGraphicsView(parent)
    , m_provider(new PdfPageProvider())
    , m_feedback(new PageItemFeedback(this))
{
    m_provider->setFeedback(new ImageSourceFeedback(this));
    // TODO: keep track QWindow::screenChanged
    m_provider->setPixelRatio(devicePixelRatio());
}

PdfView::~PdfView(){}

void PdfView::setDocument(QPdfDocument* document)
{
    m_provider->setDocument(document);

    auto* scene = new QGraphicsScene();
    scene->setBackgroundBrush(palette().brush(QPalette::Dark));

    constexpr auto documentMargins = 6;

    qreal yCursor = documentMargins;
    qreal maxPageWidth = std::numeric_limits<qreal>::min();

    for (int page = 0; page < document->pageCount(); ++page)
    {
        QSizeF pagePointSize = document->pagePointSize(page);

        const auto item = new PdfPageItem(m_provider.get(), m_feedback.get(), page);
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

void PdfView::setWheelZooming(bool enabled)
{
    m_wheelZoomingDisabled = !enabled;
}

bool PdfView::wheelZooming() const
{
    return m_wheelZoomingDisabled;
}

PdfViewSelection PdfView::getSelection() const
{
    QList<QPdfSelection> selections;

    for (const auto item : items())
        if (const auto page = dynamic_cast<PdfPageItem*>(item); page)
            selections.push_back(page->GetSelection());

    return PdfViewSelection { selections };
}

void PdfView::processLink(const QPdfLink& link)
{
    if (const auto url = link.url(); url.isValid())
        QDesktopServices::openUrl(url);
    else
    {
        const auto number = link.page();
        const auto location = link.location();
        const auto zoom = link.zoom();

        // TODO: optimize search
        for (const auto item : items())
            if (const auto page = dynamic_cast<PdfPageItem*>(item); page)
                if (page->Number() == number)
                {
                    // TODO: make zoom change respect optional
                    QTransform t = transform();
                    t.setMatrix(
                        zoom, t.m12(), t.m13(),
                        t.m21(), zoom, t.m23(),
                        t.m31(), t.m32(), t.m33()
                    );

                    setTransform(t);
                    page->ensureVisible({ location, location });
                    break;
                }
    }

    qDebug() << link.toString();
}

void PdfView::wheelEvent(QWheelEvent* event)
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

void PdfView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        for (const auto item : items())
            if (const auto page = dynamic_cast<PdfPageItem*>(item); page)
                page->SetSelectionRect({});

        m_selectionStart = mapToScene(event->pos());
    }

    QGraphicsView::mousePressEvent(event);
}

void PdfView::mouseReleaseEvent(QMouseEvent* event)
{
    m_selectionStart.reset();

    QGraphicsView::mouseReleaseEvent(event);
}

void PdfView::mouseMoveEvent(QMouseEvent* event)
{
    // NOTE: this progressive selection method is quite inefficient due to selection from the very beginning on every update.
    // TODO: provide stateful selection API to make it truly progressive.
    if (m_selectionStart)
    {
        const auto first = *m_selectionStart;
        const auto second = mapToScene(event->pos());
        const auto selectionRect = QRectF(first, second);

        for (const auto item : items())
            if (const auto page = dynamic_cast<PdfPageItem*>(item); page)
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

QGraphicsItem* PdfView::getPageItem(const int page) const
{
    return m_pageItems[page];
}


