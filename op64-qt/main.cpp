#include "qop64window.h"
#include <QtWidgets/QApplication>

#if defined(QT_STATIC)

#include <QtPlugin>

#ifdef _MSC_VER
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#else
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#endif

#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QOP64Window w;
    w.show();
    return a.exec();
}
