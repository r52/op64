#include "renderwidget.h"
#include "emulator.h"
#include <QCloseEvent>
#include "corecontrol.h"



RenderWidget::RenderWidget(Emulator* emu, QWidget* parent /*= 0*/) :
_emu(emu),
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
    if (_emu->getState() == EMU_RUNNING)
    {
        _emu->stopEmulator();
    }

    event->accept();
}

void RenderWidget::displayVI(uint64_t framerate)
{
    QMetaObject::invokeMethod(this, "setWindowTitle", Q_ARG(QString, QString("%1 VI/s").arg(framerate)));
}

RenderWidget::~RenderWidget()
{
    // Doesn't own it
    _emu = nullptr;
}
