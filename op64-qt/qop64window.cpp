#include <QFileDialog>
#include "qop64window.h"
#include "emulator.h"
#include "emulatorthread.h"
#include "logger.h"

#include <QTextEdit>
#include <QPlainTextEdit>

#ifdef _MSC_VER
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "plugins.h"
#include "configstore.h"

#include <boost/filesystem.hpp>
#include "guiconfig.h"
#include "configdialog.h"

static void stopEmulator(GLFWwindow* window)
{
    EMU.stopEmulator();
}

QOP64Window::QOP64Window(QWidget *parent)
    : QMainWindow(parent)
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

    setupGLFW();

    // register statusbar
    _plugins->setStatusBar((void*)ui.statusBar->winId());
}

QOP64Window::~QOP64Window()
{
    emuThread.quit();
    glfwTerminate();

    delete _plugins;
}

void QOP64Window::openRom(void)
{
    _romFile = QFileDialog::getOpenFileName(this, tr("Open ROM"), QString(), tr("N64 ROMS (*.v64 *.n64 *.z64"));

    if (!_romFile.isEmpty())
    {
        if (EMU.loadRom(_romFile.toLocal8Bit().data()))
        {
            glwindow = glfwCreateWindow(640, 480, "OpenGL", nullptr, nullptr);

            _plugins->setRenderWindow(glfwGetWin32Window(glwindow));
            glfwSetWindowCloseCallback(glwindow, stopEmulator);

            emuThread.start();

            emit runEmulator();
        }
    }
}

void QOP64Window::emulationFinished()
{
    glfwDestroyWindow(glwindow);
    glwindow = nullptr;
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
    EmulatorThread* emu = new EmulatorThread(_plugins);
    emu->moveToThread(&emuThread);
    connect(&emuThread, &QThread::finished, emu, &QObject::deleteLater);
    connect(emu, &EmulatorThread::emulatorFinished, this, &QOP64Window::emulationFinished);
    connect(this, &QOP64Window::runEmulator, emu, &EmulatorThread::runEmulator);
    //emuThread.start();
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
}

void QOP64Window::setupGLFW(void)
{
    // glfw init
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
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

