#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>

#include <boost/filesystem.hpp>

// op64-util
#include <oplog.h>

// local
#include "qop64window.h"
#include "emulator.h"

#include "guiconfig.h"
#include "configdialog.h"
#include "renderwidget.h"
#include "logwindow.h"

//op64core
#include <globalstrings.h>
#include <ui/configstore.h>
#include <ui/romdb.h>
#include <core/bus.h>


QOP64Window::QOP64Window(QWidget *parent)
    : QMainWindow(parent), renderWidget(nullptr)
{
    // Init logging
    oplog_init();

    ui.setupUi(this);

    setupDirectories();

    setupGUI();

    // Setup config dialog
    cfgDialog = new ConfigDialog(this);

    setupEmulationThread();
    connectGUIControls();

    // Force rom db load
    RomDB::getInstance();
}

QOP64Window::~QOP64Window()
{
    shudownEverything();
}

void QOP64Window::openRom(void)
{
    _romFile = QFileDialog::getOpenFileName(this, tr("Open ROM"), QString(), tr("N64 ROMS (*.v64 *.n64 *.z64"), 0, QFileDialog::DontUseNativeDialog);

    if (!_romFile.isEmpty())
    {
        QString romName("op64");
        if (_emu->loadRom(_romFile.toLocal8Bit().data(), romName))
        {
            renderWidget = new RenderWidget(romName, _emu);
            renderWidget->setGeometry(QRect(100, 100, 640, 480));
            renderWidget->setMinimumSize(QSize(640, 480));

            _emu->setRenderWindow(renderWidget->winId());
            renderWidget->show();

            emit runEmulator();
        }
    }
}

void QOP64Window::setupDirectories(void)
{
    using namespace GlobalStrings;

    // setup folders
    std::string gfx = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_GFX_PATH);
    std::string audio = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_AUDIO_PATH);
    std::string rsp = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_RSP_PATH);
    std::string input = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_INPUT_PATH);

    using namespace boost::filesystem;
    path gfxPath(gfx);
    path audioPath(audio);
    path rspPath(rsp);
    path inputPath(input);

    if (!exists(gfxPath))
    {
        create_directories(gfxPath);
    }

    if (!exists(audioPath))
    {
        create_directories(audioPath);
    }

    if (!exists(rspPath))
    {
        create_directories(rspPath);
    }

    if (!exists(inputPath))
    {
        create_directories(inputPath);
    }
}

void QOP64Window::setupEmulationThread(void)
{
    // Setup emulation thread
    _emu = new Emulator(this->winId());
    _emu->moveToThread(&_emuThread);
    connect(&_emuThread, &QThread::finished, _emu, &QObject::deleteLater);
    connect(_emu, &Emulator::emulatorFinished, this, &QOP64Window::emulationFinished);
    connect(this, &QOP64Window::runEmulator, _emu, &Emulator::runEmulator);
    connect(_emu, SIGNAL(stateChanged(EmuState)), this, SLOT(emulatorChangeState(EmuState)));

    _emuThread.start();

    // Log some threading info
    LOG_DEBUG(Main) << "GUI thread ID " << (uintptr_t) QThread::currentThreadId();
}

void QOP64Window::connectGUIControls(void)
{
    // gui controls

    // file
    connect(ui.actionOpen_ROM, SIGNAL(triggered(bool)), this, SLOT(openRom()));

    // emulation
    connect(ui.actionLimit_FPS, SIGNAL(toggled(bool)), _emu, SLOT(setLimitFPS(bool)), Qt::DirectConnection);
    connect(ui.actionHard_Reset, SIGNAL(triggered()), _emu, SLOT(gameHardReset()), Qt::DirectConnection);
    connect(ui.actionSoft_Reset, SIGNAL(triggered()), _emu, SLOT(gameSoftReset()), Qt::DirectConnection);
    connect(ui.actionFullscreen, SIGNAL(triggered()), this, SLOT(toggleFullscreen()));

    // options
    connect(ui.actionGraphics_Settings, SIGNAL(triggered()), _emu, SLOT(showGraphicsConfig()), Qt::DirectConnection);
    connect(ui.actionAudio_Settings, SIGNAL(triggered()), _emu, SLOT(showAudioConfig()), Qt::DirectConnection);
    connect(ui.actionInput_Settings, SIGNAL(triggered()), _emu, SLOT(showInputConfig()), Qt::DirectConnection);
    connect(ui.actionRSP_Settings, SIGNAL(triggered()), _emu, SLOT(showRSPConfig()), Qt::DirectConnection);
    connect(ui.actionEmulator_Settings, SIGNAL(triggered()), this, SLOT(openConfigDialog()));

    // advanced
    connect(ui.actionShow_Log, SIGNAL(toggled(bool)), this, SLOT(toggleShowLog(bool)));

    // about
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
}

void QOP64Window::toggleShowLog(bool show)
{
    QSettings settings(CFG_GUI_FILENAME, QSettings::IniFormat);
    settings.setValue(CFG_GUI_SHOW_LOG, show);

    if (show)
    {
        _logWindow->setGeometry(QRect(this->geometry().topRight().x(), this->geometry().topRight().y(), 640, 480));
        _logWindow->show();
    }
    else
    {
        _logWindow->hide();
    }
}

void QOP64Window::setupGUI(void)
{
    QSettings settings(CFG_GUI_FILENAME, QSettings::IniFormat);
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray(), CFG_GUI_VERSION);

    _logWindow = new LogWindow();
    _logWindow->setWindowTitle(tr("op64 Log"));
    _logWindow->setGeometry(QRect(this->geometry().topRight().x(), this->geometry().topRight().y(), 640, 480));
    _logWindow->setMinimumSize(QSize(640, 480));

    _logWindow->setLineWrapMode(QTextEdit::NoWrap);
    _logWindow->setReadOnly(true);

    bool showlog = settings.value(CFG_GUI_SHOW_LOG, false).toBool();
    ui.actionShow_Log->setChecked(showlog);

    if (showlog)
    {
        _logWindow->show();
    }
    else
    {
        _logWindow->hide();
    }

    LOG_INFO(Main) << "op64 " << OP64_VERSION << " compiled " << __DATE__;
    _logWindow->insertHtml("<br>");
}

void QOP64Window::openConfigDialog(void)
{
    cfgDialog->exec();
}

void QOP64Window::emulatorChangeState(EmuState newstate)
{
    switch (newstate)
    {
    case DEAD:
        renderWidget->deleteLater();
        renderWidget = nullptr;
        break;
    case ROM_LOADED:
        break;
    case HARDWARE_INITIALIZED:
        break;
    case EMU_RUNNING:
        ui.menuEmulation->setEnabled(true);
        break;
    case EMU_PAUSED:
        break;
    case EMU_STOPPED:
        ui.menuEmulation->setEnabled(false);
        break;
    }
}

void QOP64Window::emulationFinished()
{
    if (nullptr != renderWidget)
    {
        renderWidget->deleteLater(); renderWidget = nullptr;
    }
}

void QOP64Window::showAboutDialog(void)
{
    static QString aboutMsg =
        "op64 version " OP64_VERSION "<br/>"
#ifdef _DEBUG
        "DEBUG BUILD<br/>"
#endif
        "<br/>"
        "Experimental 64-bit emulator<br/>"
        "<br/>"
        "Core based on <a href='http://www.pj64-emu.com/'>Project 64</a> and <a href='https://code.google.com/p/mupen64plus/'>mupen64plus</a>, licensed under GPLv2,<br/>"
        "<a href='http://www.cen64.com/'>cen64</a> licensed under a custom license<br/>"
        "Configuration icons from the Human O2 icon set by <a href='http://schollidesign.deviantart.com/'>schollidesign</a>, licensed under GPL<br/>"
        "<br/>"
        "Compiled on: " __DATE__ " " __TIME__ "<br/>"
        "Compiled using: " COMPILER "<br/>"
        "Qt version: " QT_VERSION_STR "<br/>"
        "Boost version: " + QString::number(BOOST_VERSION / 100000) + "." + QString::number(BOOST_VERSION / 100 % 1000) + "." + QString::number(BOOST_VERSION % 100) + "<br/>"
        "<br/>"
        "(c) 2015<br/>"
        "Licensed under GPLv2<br/>"
        "Source code available at <a href='https://github.com/r52/op64'>GitHub</a>";

    QMessageBox::about(this, tr("About op64"), aboutMsg);
}

void QOP64Window::closeEvent(QCloseEvent* event)
{
    shudownEverything();

    QSettings settings(CFG_GUI_FILENAME, QSettings::IniFormat);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState(CFG_GUI_VERSION));

    event->accept();
}

void QOP64Window::shudownEverything(void)
{
    if (_emu->getState() == EMU_RUNNING)
    {
        _emu->stopEmulator();
        QEventLoop loop;
        connect(_emu, &Emulator::emulatorFinished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (!_emuThread.isFinished())
    {
        _emuThread.quit();
    }

    if (nullptr != renderWidget)
    {
        renderWidget->deleteLater(); renderWidget = nullptr;
    }

    if (nullptr != _logWindow)
    {
        _logWindow->deleteLater(); _logWindow = nullptr;
    }
}

void QOP64Window::toggleFullscreen(void)
{
    if (nullptr == renderWidget)
        return;

    renderWidget->toggleFullscreen();
}

