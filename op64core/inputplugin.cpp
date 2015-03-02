#include "inputplugin.h"
#include "bus.h"
#include "rom.h"

InputPlugin::InputPlugin(const char* libPath) :
_libHandle(nullptr),
_initialized(false),
_romOpened(false),
Config(nullptr),
RumbleCommand(nullptr),
GetKeys(nullptr),
ReadController(nullptr),
ControllerCommand(nullptr),
CloseDLL(nullptr),
RomOpen(nullptr),
RomClosed(nullptr),
PluginOpened(nullptr)
{
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    loadLibrary(libPath);
}

InputPlugin::~InputPlugin()
{
    close();
    unloadLibrary();
}

bool InputPlugin::initialize(void* renderWindow, void* statusBar)
{
    Bus::controllers[0].Present = FALSE;
    Bus::controllers[0].RawData = FALSE;
    Bus::controllers[0].Plugin = PLUGIN_NONE;

    Bus::controllers[1].Present = FALSE;
    Bus::controllers[1].RawData = FALSE;
    Bus::controllers[1].Plugin = PLUGIN_NONE;

    Bus::controllers[2].Present = FALSE;
    Bus::controllers[2].RawData = FALSE;
    Bus::controllers[2].Plugin = PLUGIN_NONE;

    Bus::controllers[3].Present = FALSE;
    Bus::controllers[3].RawData = FALSE;
    Bus::controllers[3].Plugin = PLUGIN_NONE;

    //Get DLL information
    void (*GetDllInfo)(PLUGIN_INFO* PluginInfo);
    GetDllInfo = (void (*)(PLUGIN_INFO*))opLibGetFunc(_libHandle, "GetDllInfo");
    if (GetDllInfo == nullptr) { return false; }

    PLUGIN_INFO PluginInfo;
    GetDllInfo(&PluginInfo);

    //Test Plugin version
    if (PluginInfo.Version == 0x0100) {
        //Get Function from DLL
        void (*InitiateControllers_1_0)(void* hMainWindow, CONTROL Controls[4]);
        InitiateControllers_1_0 = (void(*)(void*, CONTROL*))opLibGetFunc(_libHandle, "InitiateControllers");
        if (InitiateControllers_1_0 == nullptr) { return false; }
        InitiateControllers_1_0(renderWindow, Bus::controllers);
        _initialized = true;
    }

    if (PluginInfo.Version >= 0x0101) {
        typedef struct {
            void* hMainWindow;
            void* hinst;

            int MemoryBswaped;
            uint8_t* HEADER;
            CONTROL* Controls;
        } CONTROL_INFO;

        //Get Function from DLL		
        void (*InitiateControllers_1_1)(CONTROL_INFO ControlInfo);
        InitiateControllers_1_1 = (void(*)(CONTROL_INFO))opLibGetFunc(_libHandle, "InitiateControllers");
        if (InitiateControllers_1_1 == nullptr) { return false; }

        CONTROL_INFO ControlInfo;
        ControlInfo.Controls = Bus::controllers;
        ControlInfo.HEADER = Bus::rom->getImage();
        ControlInfo.hinst = GetModuleHandle(NULL);
        ControlInfo.hMainWindow = renderWindow;
        ControlInfo.MemoryBswaped = TRUE;
        InitiateControllers_1_1(ControlInfo);
        _initialized = true;
    }

    return _initialized;
}

void InputPlugin::close(void)
{
    if (_romOpened) {
        RomClosed();
        _romOpened = false;
    }
    if (_initialized) {
        CloseDLL();
        _initialized = false;
    }
}

void InputPlugin::onRomOpen(void)
{
    if (!_romOpened)
    {
        RomOpen();
        _romOpened = true;
    }
}

void InputPlugin::onRomClose(void)
{
    if (_romOpened)
    {
        RomClosed();
        _romOpened = false;
    }
}

void InputPlugin::GameReset(void)
{
    if (_romOpened)
    {
        RomClosed();
        RomOpen();
    }
}

void InputPlugin::loadLibrary(const char* libPath)
{
    unloadLibrary();

    if (!opLoadLib(&_libHandle, libPath))
    {
        LOG_ERROR("%s failed to load", libPath);
        unloadLibrary();
        return;
    }

    //Get DLL information
    void (*GetDllInfo)(PLUGIN_INFO* PluginInfo);
    GetDllInfo = (void(*)(PLUGIN_INFO*))opLibGetFunc(_libHandle, "GetDllInfo");
    if (GetDllInfo == nullptr)
    {
        LOG_ERROR("%s: invalid plugin", libPath);
        unloadLibrary();
        return;
    }

    GetDllInfo(&_pluginInfo);
    if (!Plugins::ValidPluginVersion(_pluginInfo))
    {
        LOG_ERROR("%s: unsupported plugin version", libPath);
        unloadLibrary();
        return;
    }

    //Find entries for functions in DLL
    void (*InitFunc)(void);
    Config = (void(*)(void*))opLibGetFunc(_libHandle, "DllConfig");
    ControllerCommand = (void(*)(int, uint8_t*))opLibGetFunc(_libHandle, "ControllerCommand");
    GetKeys = (void(*)(int, BUTTONS*)) opLibGetFunc(_libHandle, "GetKeys");
    ReadController = (void(*)(int, uint8_t*))opLibGetFunc(_libHandle, "ReadController");
    InitFunc = (void(*)(void)) opLibGetFunc(_libHandle, "InitiateControllers");
    RomOpen = (void(*)(void)) opLibGetFunc(_libHandle, "RomOpen");
    RomClosed = (void(*)(void)) opLibGetFunc(_libHandle, "RomClosed");
    CloseDLL = (void(*)(void)) opLibGetFunc(_libHandle, "CloseDLL");
    RumbleCommand = (void(*)(int, int))opLibGetFunc(_libHandle, "RumbleCommand");

    //version 101 functions
    PluginOpened = (void(*)(void))opLibGetFunc(_libHandle, "PluginLoaded");

    //Make sure dll had all needed functions
    if (InitFunc == nullptr) { unloadLibrary(); return; }
    if (CloseDLL == nullptr) { unloadLibrary(); return; }

    if (_pluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == nullptr) { unloadLibrary(); return; }

        PluginOpened();
    }
}

void InputPlugin::unloadLibrary(void)
{
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));

    if (nullptr != _libHandle) {
        opLibClose(_libHandle);
        _libHandle = nullptr;
    }

    Config = nullptr;
    ControllerCommand = nullptr;
    GetKeys = nullptr;
    ReadController = nullptr;
    CloseDLL = nullptr;
    RomOpen = nullptr;
    RomClosed = nullptr;
}
