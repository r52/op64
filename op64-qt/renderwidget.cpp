#include "renderwidget.h"
#include "emulator.h"
#include <QCloseEvent>
#include "corecontrol.h"



RenderWidget::RenderWidget(QWidget* parent /*= 0*/) :
QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    CoreControl::displayVI = std::bind(&RenderWidget::displayVI, this, std::placeholders::_1);
}

void RenderWidget::closeEvent(QCloseEvent * event)
{
    if (EMU.getState() == EMU_RUNNING)
    {
        EMU.stopEmulator();
    }

    event->accept();
}

void RenderWidget::displayVI(uint64_t framerate)
{
    QMetaObject::invokeMethod(this, "setWindowTitle", Q_ARG(QString, QString("%1 VI/s").arg(framerate)));
}
