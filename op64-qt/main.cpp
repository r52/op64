#include "qop64window.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QOP64Window w;
    w.show();
    return a.exec();
}
