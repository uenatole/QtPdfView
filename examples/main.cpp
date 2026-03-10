#include <QApplication>
#include <QPdfDocument>
#include <QShortcut>

#include "ui/DocumentView.h"
#include "ui/addons/DocumentSelector.h"
#include "ui/addons/DocumentZoomer.h"

#include "core/DocumentFacade.h"
#include "core/StandardDocumentParser.h"
#include "core/StandardDocumentRenderer.h"

#include "backend/pdf/PdfDocument.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const auto pdf = std::make_shared<PdfDocument>();
    pdf->load(qEnvironmentVariable("DOCUMENT"));

    const auto renderer = std::make_shared<StandardDocumentRenderer>();
    const auto parser = std::make_shared<StandardDocumentParser>();

    const auto document = std::make_shared<DocumentFacade>();
    document->setDocument(pdf);
    document->setRenderer(renderer);
    document->setParser(parser);

    DocumentView view;

    DocumentSelector selector(&view);
    DocumentZoomer zoomer(&view);

    const QShortcut copyShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), &view);
    QObject::connect(&copyShortcut, &QShortcut::activated, [&]
    {
        QGuiApplication::clipboard()->setText(view.getSelectedText(), QClipboard::Clipboard);
    });

    view.setDocument(document);
    view.show();

    return QApplication::exec();
}
