#include "qop64window.h"
#include "emulator.h"
#include "logger.h"
#include "version.h"
#include "plugins.h"
#include "configstore.h"

#include <QFileDialog>
#include <QTextEdit>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>

#include <boost/filesystem.hpp>
#include "guiconfig.h"
#include "configdialog.h"
#include "renderwidget.h"


static const char* logLevelFormatting[LOG_LEVEL_NUM] = {
    "<font color='mediumblue'><b>%1<b></font><br>",
    "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;%1<br>",
    "<font color='forestgreen'><b>%1</font><br>",
    "<font color='orange'><b>%1<b></font><br>",
    "<font color='maroon'><b>%1</b></font><br>"
};

QOP64Window::QOP64Window(QWidget *parent)
    : QMainWindow(parent), renderWidget(nullptr)
{
    ui.setupUi(this);

    setupDirectories();

    setupGUI();
    // Set up log callback
    LOG.setLogCallback(std::bind(&QOP64Window::logCallback, this, std::placeholders::_1, std::placeholders::_2));

    LOG_INFO("op64 %s compiled %s", OP64_VERSION, __DATE__);
    _logWindow->insertHtml("<br>");

    // Setup plugins
    _plugins = new Plugins();

    // Setup config dialog
    cfgDialog = new ConfigDialog(_plugins, this);

    setupEmulationThread();
    connectGUIControls();
}

QOP64Window::~QOP64Window()
{
    shudownEverything();
}

void QOP64Window::openRom(void)
{
    _romFile = QFileDialog::getOpenFileName(this, tr("Open ROM"), QString(), tr("N64 ROMS (*.v64 *.n64 *.z64"));

    if (!_romFile.isEmpty())
    {
        if (_emu->loadRom(_romFile.toLocal8Bit().data()))
        {
            renderWidget = new RenderWidget(_emu);
            renderWidget->setGeometry(QRect(100, 100, 640, 480));
            renderWidget->setMinimumSize(QSize(640, 480));

            _plugins->setRenderWindow((void*)renderWidget->winId());
            renderWidget->show();

            emit runEmulator();
        }
    }
}

void QOP64Window::logCallback(uint32_t level, const char* msg)
{
    QMetaObject::invokeMethod(_logWindow, "insertHtml", Q_ARG(QString, QString(logLevelFormatting[level]).arg(QString(msg))));
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
    _emu = new Emulator(_plugins);
    _emu->moveToThread(&_emuThread);
    connect(&_emuThread, &QThread::finished, _emu, &QObject::deleteLater);
    connect(_emu, &Emulator::emulatorFinished, this, &QOP64Window::emulationFinished);
    connect(this, &QOP64Window::runEmulator, _emu, &Emulator::runEmulator);
    connect(_emu, SIGNAL(stateChanged(EmuState)), this, SLOT(emulatorChangeState(EmuState)), Qt::DirectConnection);

    _emuThread.start();
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

    // options
    connect(ui.actionGraphics_Settings, SIGNAL(triggered()), this, SLOT(showGraphicsConfig()));
    connect(ui.actionAudio_Settings, SIGNAL(triggered()), this, SLOT(showAudioConfig()));
    connect(ui.actionInput_Settings, SIGNAL(triggered()), this, SLOT(showInputConfig()));
    connect(ui.actionRSP_Settings, SIGNAL(triggered()), this, SLOT(showRSPConfig()));
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

    _logWindow = new QTextEdit();
    _logWindow->moveToThread(&_emuThread);
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
        QMetaObject::invokeMethod(ui.menuEmulation, "setEnabled", Q_ARG(bool, true));
        break;
    case EMU_PAUSED:
        break;
    case EMU_STOPPED:
        QMetaObject::invokeMethod(ui.menuEmulation, "setEnabled", Q_ARG(bool, false));
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

    _emuThread.quit();

    if (nullptr != _plugins)
    {
        delete _plugins; _plugins = nullptr;
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

