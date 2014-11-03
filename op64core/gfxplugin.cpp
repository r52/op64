#include "gfxplugin.h"
#include "bus.h"
#include "rcpcommon.h"


void gfx_dummy_func(void) {}
void DummyMoveScreen(int32_t, int32_t) {}

GfxPlugin::GfxPlugin(const char* libPath) :
CaptureScreen(nullptr),
ChangeWindow(nullptr),
Config(nullptr),
DrawScreen(nullptr),
MoveScreen(nullptr),
ProcessDList(nullptr),
ProcessRDPList(nullptr),
ShowCFB(nullptr),
UpdateScreen(nullptr),
ViStatusChanged(nullptr),
ViWidthChanged(nullptr),
SoftReset(nullptr),
CloseDLL(nullptr),
RomOpen(nullptr),
RomClosed(nullptr),
PluginOpened(nullptr),
fbGetInfo(nullptr),
fbRead(nullptr),
fbWrite(nullptr),
_libHandle(nullptr),
_initialized(false),
_romOpened(false)
{
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    loadLibrary(libPath);
}

GfxPlugin::~GfxPlugin()
{
    close();
    unloadLibrary();
}

void GfxPlugin::close(void)
{
    if (_romOpened)
    {
        onRomClose();
    }

    if (_initialized)
    {
        CloseDLL();
        _initialized = false;
    }
}

void GfxPlugin::unloadLibrary(void)
{
    if (nullptr != _libHandle) {
        opLibClose(_libHandle);
        _libHandle = nullptr;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));

    //	CaptureScreen = NULL;
    ChangeWindow = nullptr;
    CloseDLL = nullptr;
    //	DllAbout = NULL;
    Config = nullptr;
    RomClosed = nullptr;
    RomOpen = nullptr;
    DrawScreen = nullptr;
    MoveScreen = nullptr;
    ProcessDList = nullptr;
    ProcessRDPList = nullptr;
    ShowCFB = nullptr;
    UpdateScreen = nullptr;
    ViStatusChanged = nullptr;
    ViWidthChanged = nullptr;
    fbGetInfo = nullptr;
    fbRead = nullptr;
    fbWrite = nullptr;
}

void GfxPlugin::loadLibrary(const char* libPath)
{
    if (!opLoadLib(&_libHandle, libPath))
    {
        LOG_ERROR("%s failed to load", libPath);
        unloadLibrary();
        return;
    }

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

    int (*InitFunc)(void * Gfx_Info);
    InitFunc = (int(*)(void *)) opLibGetFunc(_libHandle, "InitiateGFX");
    CloseDLL = (void(*)(void)) opLibGetFunc(_libHandle, "CloseDLL");
    ChangeWindow = (void(*)(void)) opLibGetFunc(_libHandle, "ChangeWindow");
    Config = (void(*)(void*)) opLibGetFunc(_libHandle, "DllConfig");
    DrawScreen = (void(*)(void)) opLibGetFunc(_libHandle, "DrawScreen");
    MoveScreen = (void(*)(int32_t, int32_t))opLibGetFunc(_libHandle, "MoveScreen");
    ProcessDList = (void(*)(void)) opLibGetFunc(_libHandle, "ProcessDList");
    RomClosed = (void(*)(void)) opLibGetFunc(_libHandle, "RomClosed");
    RomOpen = (void(*)(void)) opLibGetFunc(_libHandle, "RomOpen");
    UpdateScreen = (void(*)(void)) opLibGetFunc(_libHandle, "UpdateScreen");
    ViStatusChanged = (void(*)(void)) opLibGetFunc(_libHandle, "ViStatusChanged");
    ViWidthChanged = (void(*)(void)) opLibGetFunc(_libHandle, "ViWidthChanged");
    SoftReset = (void(*)(void)) opLibGetFunc(_libHandle, "SoftReset");

    if (ChangeWindow == nullptr)    { unloadLibrary(); return; }
    if (DrawScreen == nullptr)      { DrawScreen = gfx_dummy_func; }
    if (InitFunc == nullptr)        { unloadLibrary(); return; }
    if (MoveScreen == nullptr)      { MoveScreen = DummyMoveScreen; }
    if (ProcessDList == nullptr)    { unloadLibrary(); return; }
    if (RomClosed == nullptr)       { unloadLibrary(); return; }
    if (RomOpen == nullptr)         { unloadLibrary(); return; }
    if (UpdateScreen == nullptr)    { unloadLibrary(); return; }
    if (ViStatusChanged == nullptr) { ViStatusChanged = gfx_dummy_func; }
    if (ViWidthChanged == nullptr)  { ViWidthChanged = gfx_dummy_func; }
    if (CloseDLL == nullptr)        { unloadLibrary(); return; }
    if (SoftReset == nullptr)       { SoftReset = gfx_dummy_func; }

    if (_pluginInfo.Version >= 0x0103){
        ProcessRDPList = (void(*)(void))opLibGetFunc(_libHandle, "ProcessRDPList");
        CaptureScreen = (void(*)(const char *))opLibGetFunc(_libHandle, "CaptureScreen");
        ShowCFB = (void(*)(void))opLibGetFunc(_libHandle, "ShowCFB");

        if (ProcessRDPList == nullptr) { unloadLibrary(); return; }
        if (CaptureScreen == nullptr)  { unloadLibrary(); return; }
        if (ShowCFB == nullptr)        { unloadLibrary(); return; }
    }

    if (_pluginInfo.Version >= 0x0104)
    {
        if (PluginOpened == nullptr) { unloadLibrary(); return; }

        PluginOpened();
    }

    // frame buffer extension
    fbRead = (void(*)(uint32_t)) opLibGetFunc(_libHandle, "FBRead");
    fbWrite = (void(*)(uint32_t, uint32_t)) opLibGetFunc(_libHandle, "FBWrite");
    fbGetInfo = (void(*)(void*)) opLibGetFunc(_libHandle, "FBGetFrameBufferInfo");
}

void GfxPlugin::onRomOpen(void)
{
    if (!_romOpened)
    {
        RomOpen();
        _romOpened = true;
    }
}

void GfxPlugin::onRomClose(void)
{
    if (_romOpened)
    {
        RomClosed();
        _romOpened = false;
    }
}

void GfxPlugin::GameReset(void)
{
    if (_romOpened)
    {
        onRomClose();
        onRomOpen();
    }
}

bool GfxPlugin::initialize(void* renderWindow, void* statusBar)
{
    if (_initialized)
    {
        close();
    }

    // TODO future: different spec for linux (mupen spec)
    typedef struct {
        void* hWnd;
        void* hStatusBar;

        int MemoryBswaped;
        uint8_t* HEADER;
        uint8_t* RDRAM;
        uint8_t* DMEM;
        uint8_t* IMEM;

        uint32_t* MI_INTR_REG;

        uint32_t* DPC_START_REG;
        uint32_t* DPC_END_REG;
        uint32_t* DPC_CURRENT_REG;
        uint32_t* DPC_STATUS_REG;
        uint32_t* DPC_CLOCK_REG;
        uint32_t* DPC_BUFBUSY_REG;
        uint32_t* DPC_PIPEBUSY_REG;
        uint32_t* DPC_TMEM_REG;

        uint32_t* VI_STATUS_REG;
        uint32_t* VI_ORIGIN_REG;
        uint32_t* VI_WIDTH_REG;
        uint32_t* VI_INTR_REG;
        uint32_t* VI_V_CURRENT_LINE_REG;
        uint32_t* VI_TIMING_REG;
        uint32_t* VI_V_SYNC_REG;
        uint32_t* VI_H_SYNC_REG;
        uint32_t* VI_LEAP_REG;
        uint32_t* VI_H_START_REG;
        uint32_t* VI_V_START_REG;
        uint32_t* VI_V_BURST_REG;
        uint32_t* VI_X_SCALE_REG;
        uint32_t* VI_Y_SCALE_REG;

        void (*CheckInterrupts)(void);
    } GFX_INFO;

    int (*InitiateGFX)(GFX_INFO Gfx_Info);
    InitiateGFX = (int(*)(GFX_INFO))opLibGetFunc(_libHandle, "InitiateGFX");
    if (InitiateGFX == nullptr)
    {
        return false;
    }

    GFX_INFO Info;
    memset(&Info, 0, sizeof(Info));

    Info.MemoryBswaped = TRUE;
    Info.CheckInterrupts = DummyFunction;
    Info.hWnd = renderWindow;
    Info.hStatusBar = statusBar;

    Info.HEADER = Bus::rom_image;
    Info.RDRAM = Bus::rdram8;
    Info.DMEM = Bus::sp_dmem8;
    Info.IMEM = Bus::sp_imem8;

    Info.MI_INTR_REG = &Bus::mi_reg[MI_INTR_REG];

    Info.DPC_START_REG = &Bus::dp_reg[DPC_START_REG];
    Info.DPC_END_REG = &Bus::dp_reg[DPC_END_REG];
    Info.DPC_CURRENT_REG = &Bus::dp_reg[DPC_CURRENT_REG];
    Info.DPC_STATUS_REG = &Bus::dp_reg[DPC_STATUS_REG];
    Info.DPC_CLOCK_REG = &Bus::dp_reg[DPC_CLOCK_REG];
    Info.DPC_BUFBUSY_REG = &Bus::dp_reg[DPC_BUFBUSY_REG];
    Info.DPC_PIPEBUSY_REG = &Bus::dp_reg[DPC_PIPEBUSY_REG];
    Info.DPC_TMEM_REG = &Bus::dp_reg[DPC_TMEM_REG];

    Info.VI_STATUS_REG = &Bus::vi_reg[VI_STATUS_REG];
    Info.VI_ORIGIN_REG = &Bus::vi_reg[VI_ORIGIN_REG];
    Info.VI_WIDTH_REG = &Bus::vi_reg[VI_WIDTH_REG];
    Info.VI_INTR_REG = &Bus::vi_reg[VI_INTR_REG];
    Info.VI_V_CURRENT_LINE_REG = &Bus::vi_reg[VI_CURRENT_REG];
    Info.VI_TIMING_REG = &Bus::vi_reg[VI_BURST_REG];
    Info.VI_V_SYNC_REG = &Bus::vi_reg[VI_V_SYNC_REG];
    Info.VI_H_SYNC_REG = &Bus::vi_reg[VI_H_SYNC_REG];
    Info.VI_LEAP_REG = &Bus::vi_reg[VI_LEAP_REG];
    Info.VI_H_START_REG = &Bus::vi_reg[VI_H_START_REG];
    Info.VI_V_START_REG = &Bus::vi_reg[VI_V_START_REG];
    Info.VI_V_BURST_REG = &Bus::vi_reg[VI_V_BURST_REG];
    Info.VI_X_SCALE_REG = &Bus::vi_reg[VI_X_SCALE_REG];
    Info.VI_Y_SCALE_REG = &Bus::vi_reg[VI_Y_SCALE_REG];

    _initialized = (InitiateGFX(Info) != 0);

    return _initialized;
}
