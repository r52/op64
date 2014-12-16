#include "qop64window.h"
#include <QtWidgets/QApplication>

#if defined(QT_STATIC) && defined(_MSC_VER)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QOP64Window w;
    w.show();
    return a.exec();
}
