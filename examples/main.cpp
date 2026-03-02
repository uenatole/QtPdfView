#include <QApplication>
#include <QPdfDocument>
#include <QShortcut>

#include "PdfView.h"
#include "core/Document.h"
#include "backend/pdf/PdfDocumentParser.h"
#include "backend/pdf/PdfDocumentRenderer.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    const auto pdf = std::make_shared<QPdfDocument>();
    pdf->load(qEnvironmentVariable("DOCUMENT"));

    const auto renderer = std::make_shared<PdfDocumentRenderer>(pdf);
    const auto parser = std::make_shared<PdfDocumentParser>(pdf);

    const auto document = std::make_shared<Document>();
    document->setImageSource(renderer);
    document->setTextSource(parser);
    document->setLinkSource(parser);
    document->setMetaSource(parser);

    PdfView view;

    const QShortcut copyShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), &view);
    QObject::connect(&copyShortcut, &QShortcut::activated, [&]
    {
        QGuiApplication::clipboard()->setText(view.getSelectedText(), QClipboard::Clipboard);
    });

    view.setDocument(document);
    view.setWheelZooming(true);
    view.show();

    return QApplication::exec();
}
