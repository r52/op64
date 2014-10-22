#include <QFileDialog>
#include "qop64window.h"
#include "emulator.h"
#include "emulatorthread.h"
#include "logger.h"

#include <QTextEdit>
#include <QPlainTextEdit>

#include "plugins.h"
#include "configstore.h"

#include <boost/filesystem.hpp>
#include "guiconfig.h"
#include "configdialog.h"

#include "renderwidget.h"
#include <QMessageBox>


QOP64Window::QOP64Window(QWidget *parent)
    : QMainWindow(parent), renderWidget(nullptr)
{
    ui.setupUi(this);

    // Setup plugins
    _plugins = new Plugins();

    // Setup config dialog
    cfgDialog = new ConfigDialog(_plugins);

    setupDirectories();

    // Set up log callback
    LOG.setLogCallback(std::bind(&QOP64Window::logCallback, this, std::placeholders::_1));

    setupEmulationThread();

    setupGUI();
    connectGUIControls();


    // register statusbar
    //_plugins->setStatusBar((void*)ui.statusBar->winId());
}

QOP64Window::~QOP64Window()
{
    if (EMU.getState() == EMU_RUNNING)
    {
        EMU.stopEmulator();
        QEventLoop loop;
        connect(_emu, &EmulatorThread::emulatorFinished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    _emuThread.quit();

    delete _plugins;

    if (nullptr != renderWidget)
    {
        renderWidget->deleteLater(); renderWidget = nullptr;
    }
}

void QOP64Window::openRom(void)
{
    _romFile = QFileDialog::getOpenFileName(this, tr("Open ROM"), QString(), tr("N64 ROMS (*.v64 *.n64 *.z64"));

    if (!_romFile.isEmpty())
    {
        if (EMU.loadRom(_romFile.toLocal8Bit().data()))
        {
            renderWidget = new RenderWidget;
            renderWidget->setGeometry(QRect(0, 0, 640, 480));
            renderWidget->setMinimumSize(QSize(640, 480));
            renderWidget->move(100, 100);

            _plugins->setRenderWindow((void*)renderWidget->winId());
            renderWidget->show();

            emit runEmulator();
        }
    }
}

void QOP64Window::logCallback(const char* msg)
{
    ui.debugEdit->insertPlainText(QString(msg));
}

void QOP64Window::setupDirectories(void)
{
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
    _emu = new EmulatorThread(_plugins);
    _emu->moveToThread(&_emuThread);
    connect(&_emuThread, &QThread::finished, _emu, &QObject::deleteLater);
    connect(_emu, &EmulatorThread::emulatorFinished, this, &QOP64Window::emulationFinished);
    connect(this, &QOP64Window::runEmulator, _emu, &EmulatorThread::runEmulator);

    EMU.moveToThread(&_emuThread);
    connect(&EMU, SIGNAL(stateChanged(EmuState)), this, SLOT(emulatorChangeState(EmuState)), Qt::DirectConnection);

    _emuThread.start();
}

void QOP64Window::connectGUIControls(void)
{
    // gui controls

    // file
    connect(ui.actionOpen_ROM, SIGNAL(triggered(bool)), this, SLOT(openRom()));

    // options
    connect(ui.actionGraphics_Settings, SIGNAL(triggered(bool)), this, SLOT(showGraphicsConfig()));
    connect(ui.actionAudio_Settings, SIGNAL(triggered(bool)), this, SLOT(showAudioConfig()));
    connect(ui.actionInput_Settings, SIGNAL(triggered(bool)), this, SLOT(showInputConfig()));
    connect(ui.actionRSP_Settings, SIGNAL(triggered(bool)), this, SLOT(showRSPConfig()));
    connect(ui.actionEmulator_Settings, SIGNAL(triggered()), this, SLOT(openConfigDialog()));

    // advanced
    connect(ui.actionShow_Log, SIGNAL(toggled(bool)), this, SLOT(toggleShowLog(bool)));

    // about
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
}

void QOP64Window::toggleShowLog(bool show)
{
    ConfigStore::getInstance().set(CFG_SECTION_GUI, CFG_GUI_SHOW_LOG, show);
    if (show)
    {
        ui.debugEdit->show();
    }
    else
    {
        ui.debugEdit->hide();
    }
}

void QOP64Window::setupGUI(void)
{
    ui.debugEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    ui.debugEdit->setReadOnly(true);

    bool showlog = ConfigStore::getInstance().getBool(CFG_SECTION_GUI, CFG_GUI_SHOW_LOG);
    ui.actionShow_Log->setChecked(showlog);

    if (showlog)
    {
        ui.debugEdit->show();
    }
    else
    {
        ui.debugEdit->hide();
    }
}

void QOP64Window::openConfigDialog(void)
{
    cfgDialog->exec();
}

void QOP64Window::showAudioConfig(void)
{
    _plugins->ConfigPlugin((void*)this->winId(), PLUGIN_TYPE_AUDIO);
}

void QOP64Window::showInputConfig(void)
{
    _plugins->ConfigPlugin((void*)this->winId(), PLUGIN_TYPE_CONTROLLER);
}

void QOP64Window::showGraphicsConfig(void)
{
    _plugins->ConfigPlugin((void*)this->winId(), PLUGIN_TYPE_GFX);
}

void QOP64Window::showRSPConfig(void)
{
    _plugins->ConfigPlugin((void*)this->winId(), PLUGIN_TYPE_RSP);
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
        break;
    case EMU_PAUSED:
        break;
    case EMU_STOPPED:
        break;
    case HARDWARE_DEINITIALIZED:
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
        "op64<br/>"
#ifdef _DEBUG
        "DEBUG BUILD<br/>"
#endif
        "<br/>"
        "Experimental 64-bit emulator<br/>"
        "<br/>"
        "Core based on <a href='http://www.pj64-emu.com/'>Project 64</a> and <a href='https://code.google.com/p/mupen64plus/'>mupen64plus</a><br/>"
        "Configuration icons from the Human O2 icon set by <a href='http://schollidesign.deviantart.com/'>schollidesign</a>, licensed under GPL<br/>"
        "<br/>"
        "Compiled on: " __DATE__ " " __TIME__ "<br/>"
        "Compiled using: " COMPILER "<br/>"
        "Qt version: " QT_VERSION_STR "<br/>"
        "Boost version: " + QString::number(BOOST_VERSION / 100000) + "." + QString::number(BOOST_VERSION / 100 % 1000) + "." + QString::number(BOOST_VERSION % 100) + "<br/>"
        "<br/>"
        "(c) 2014<br/>"
        "Licensed under GPLv2<br/>"
        "Source code available at <a href=''>GitHub</a>";

    QMessageBox::about(this, tr("About op64"), aboutMsg);
}

