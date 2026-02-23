#include <QApplication>
#include <QPdfDocument>
#include <QShortcut>

#include "PdfView.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    PdfView view;
    QPdfDocument document;
    document.load(qEnvironmentVariable("DOCUMENT"));

    const QShortcut copyShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), &view);
    QObject::connect(&copyShortcut, &QShortcut::activated, [&]
    {
        QGuiApplication::clipboard()->setText(view.getSelectedText(), QClipboard::Clipboard);
    });

    view.setDocument(&document);
    view.setWheelZooming(true);
    view.show();

    return QApplication::exec();
}
