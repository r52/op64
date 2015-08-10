#include <QTimer>
#include <QEventLoop>
#include <QCloseEvent>
#include <QShortcut>

#include "renderwidget.h"
#include "emulator.h"

#include <ui/corecontrol.h>
#include <rom/rom.h>
#include <core/bus.h>


RenderWidget::RenderWidget(Emulator* emu, QWidget* parent /*= 0*/) :
_emu(emu),
QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(displayVI()));
    timer->start(1000);

    romName = QString::fromLocal8Bit((char*)Bus::rom->getHeader()->Name, 20);
    romName = romName.trimmed();
    setWindowTitle(romName);

    QShortcut* flscrn = new QShortcut(QKeySequence("F11"), this);
    connect(flscrn, SIGNAL(activated()), this, SLOT(toggleFullscreen()));
}

void RenderWidget::closeEvent(QCloseEvent * event)
{
    if (_emu->getState() == EMU_RUNNING)
    {
        _emu->stopEmulator();
        QEventLoop loop;
        connect(_emu, &Emulator::emulatorFinished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    event->accept();
}

void RenderWidget::displayVI()
{
    static QString title("%1 | %2 VI/s");
    QMetaObject::invokeMethod(this, "setWindowTitle", Q_ARG(QString, title.arg(romName).arg(QString::number(CoreControl::fps.load(), 'f', 2))));
}

RenderWidget::~RenderWidget()
{
    // Doesn't own it
    _emu = nullptr;
}

void RenderWidget::toggleFullscreen(void)
{
    // FIXME: buggy (no border when fullscreen -> normal on glide64)
    _emu->toggleFullScreen();

    if (isFullScreen())
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}
