#include <QApplication>
#include <QPdfDocument>

#include "PdfView.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    PdfView view;
    QPdfDocument document;
    document.load(qEnvironmentVariable("DOCUMENT"));

    view.setDocument(&document);
    view.setWheelZooming(true);
    view.show();

    return QApplication::exec();
}
