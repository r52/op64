#include "renderwidget.h"
#include "emulator.h"
#include <QCloseEvent>



RenderWidget::RenderWidget(QWidget* parent /*= 0*/) :
QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void RenderWidget::closeEvent(QCloseEvent * event)
{
    if (EMU.getState() == EMU_RUNNING)
    {
        EMU.stopEmulator();
    }

    event->accept();
}
