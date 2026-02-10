# QPdfView, but QGraphicsView-based

The main problem of QPdfView (Qt::PdfWidgets class) is reinventing panning, zooming and other transformations that are already present in Qt, but one step over QAbstractScrollArea the QPdfView is based - it's QGraphicsView.

Pros:
- no need to implement zooming, panning, transformation anchors required by any functional PDF viewer;
- layout creation is more pleasant with QGraphicsScene + QGraphicsItem;
- drawing is done on the QGraphicsView side, so you can focus on the core logic first.

Cons:
- see "Issues".
