#include "gfxplugin.h"

#include <oplog.h>
#include <core/bus.h>
#include <rcp/rcpcommon.h>
#include <rcp/rcp.h>
#include <rom/rom.h>


static void gfx_dummy_func(void) {}
static void DummyMoveScreen(int32_t, int32_t) {}

GfxPlugin::GfxPlugin() :
    CaptureScreen(nullptr),
    ChangeWindow(nullptr),
    DrawScreen(nullptr),
    MoveScreen(nullptr),
    ProcessDList(nullptr),
    ProcessRDPList(nullptr),
    ShowCFB(nullptr),
    UpdateScreen(nullptr),
    ViStatusChanged(nullptr),
    ViWidthChanged(nullptr),
    SoftReset(nullptr),
    fbGetInfo(nullptr),
    fbRead(nullptr),
    fbWrite(nullptr)
{
}

GfxPlugin::~GfxPlugin()
{
    closePlugin();
    unloadPlugin();
}

OPStatus GfxPlugin::unloadPlugin()
{
    if (OP_ERROR == freeLibrary(_libHandle))
    {
        LOG_FATAL(GfxPlugin) << "Error unloading plugin";
        return OP_ERROR;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));

    //	CaptureScreen = NULL;
    ChangeWindow = nullptr;
    CloseLib = nullptr;
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

    return OP_OK;
}


OPStatus GfxPlugin::loadPlugin(const char* libPath, GfxPlugin*& outplug)
{
    if (nullptr != outplug)
    {
        LOG_DEBUG(GfxPlugin) << "Existing Plugin object";
        return OP_ERROR;
    }

    GfxPlugin* plugin = new GfxPlugin;

    if (OP_ERROR == loadLibrary(libPath, plugin->_libHandle, plugin->_pluginInfo))
    {
        delete plugin;
        LOG_ERROR(GfxPlugin) << "Error loading library " << libPath;
        return OP_ERROR;
    }

    //Find entries for functions in DLL
    int(*InitFunc)(void * Gfx_Info);

    plugin->loadDefaults();
    plugin->loadPJ64Settings();

    getPluginFunction(plugin->_libHandle, "InitiateGFX", InitFunc);
    getPluginFunction(plugin->_libHandle, "ChangeWindow", plugin->ChangeWindow);
    getPluginFunction(plugin->_libHandle, "DrawScreen", plugin->DrawScreen);
    getPluginFunction(plugin->_libHandle, "MoveScreen", plugin->MoveScreen);
    getPluginFunction(plugin->_libHandle, "ProcessDList", plugin->ProcessDList);
    getPluginFunction(plugin->_libHandle, "UpdateScreen", plugin->UpdateScreen);
    getPluginFunction(plugin->_libHandle, "ViStatusChanged", plugin->ViStatusChanged);
    getPluginFunction(plugin->_libHandle, "ViWidthChanged", plugin->ViWidthChanged);
    getPluginFunction(plugin->_libHandle, "SoftReset", plugin->SoftReset);

    //version 102 functions
    getPluginFunction(plugin->_libHandle, "PluginLoaded", plugin->PluginOpened);

    if (InitFunc == nullptr ||
        plugin->ChangeWindow == nullptr ||
        plugin->ProcessDList == nullptr ||
        plugin->RomClosed == nullptr ||
        plugin->RomOpen == nullptr ||
        plugin->UpdateScreen == nullptr ||
        plugin->CloseLib == nullptr)
    {
        freeLibrary(plugin->_libHandle);
        delete plugin;
        LOG_ERROR(GfxPlugin) << "Invalid plugin: not all required functions are found";
        return OP_ERROR;
    }

    if (plugin->DrawScreen == nullptr) { plugin->DrawScreen = gfx_dummy_func; }
    if (plugin->MoveScreen == nullptr) { plugin->MoveScreen = DummyMoveScreen; }
    if (plugin->ViStatusChanged == nullptr) { plugin->ViStatusChanged = gfx_dummy_func; }
    if (plugin->ViWidthChanged == nullptr) { plugin->ViWidthChanged = gfx_dummy_func; }
    if (plugin->SoftReset == nullptr) { plugin->SoftReset = gfx_dummy_func; }

    if (plugin->_pluginInfo.Version >= 0x0103)
    {
        getPluginFunction(plugin->_libHandle, "ProcessRDPList", plugin->ProcessRDPList);
        getPluginFunction(plugin->_libHandle, "CaptureScreen", plugin->CaptureScreen);
        getPluginFunction(plugin->_libHandle, "ShowCFB", plugin->ShowCFB);

        if (plugin->ProcessRDPList == nullptr ||
            plugin->CaptureScreen == nullptr ||
            plugin->ShowCFB == nullptr)
        {
            freeLibrary(plugin->_libHandle);
            delete plugin;
            LOG_ERROR(GfxPlugin) << "Invalid plugin: not all required functions are found";
            return OP_ERROR;
        }
    }

    if (plugin->_pluginInfo.Version >= 0x0104)
    {
        if (plugin->PluginOpened == nullptr)
        {
            freeLibrary(plugin->_libHandle);
            delete plugin;
            LOG_ERROR(GfxPlugin) << "Invalid plugin: not all required functions are found";
            return OP_ERROR;
        }

        plugin->PluginOpened();
    }

    // frame buffer extension
    getPluginFunction(plugin->_libHandle, "FBRead", plugin->fbRead);
    getPluginFunction(plugin->_libHandle, "FBWrite", plugin->fbWrite);
    getPluginFunction(plugin->_libHandle, "FBGetFrameBufferInfo", plugin->fbGetInfo);

    // Return it
    outplug = plugin; plugin = nullptr;

    return OP_OK;
}

OPStatus GfxPlugin::initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar)
{
    if (_initialized)
    {
        closePlugin();
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

    getPluginFunction(_libHandle, "InitiateGFX", InitiateGFX);
    if (InitiateGFX == nullptr)
    {
        LOG_ERROR(GfxPlugin) << "InitiateGFX not found";
        return OP_ERROR;
    }

    GFX_INFO Info;
    memset(&Info, 0, sizeof(Info));

    Info.MemoryBswaped = 1;
    Info.CheckInterrupts = DummyFunction;
    Info.hWnd = renderWindow;
    Info.hStatusBar = statusBar;

    if (bus && bus->rom)
    {
        Info.HEADER = bus->rom->getImage();
    }
    else
    {
        uint8_t buf[100];
        Info.HEADER = buf;
    }
    Info.RDRAM = (uint8_t*)Bus::rdram.mem;
    Info.DMEM = (uint8_t*)Bus::rcp.sp.dmem;
    Info.IMEM = (uint8_t*)Bus::rcp.sp.imem;

    Info.MI_INTR_REG = &Bus::rcp.mi.reg[MI_INTR_REG];

    Info.DPC_START_REG = &Bus::rcp.dpc.reg[DPC_START_REG];
    Info.DPC_END_REG = &Bus::rcp.dpc.reg[DPC_END_REG];
    Info.DPC_CURRENT_REG = &Bus::rcp.dpc.reg[DPC_CURRENT_REG];
    Info.DPC_STATUS_REG = &Bus::rcp.dpc.reg[DPC_STATUS_REG];
    Info.DPC_CLOCK_REG = &Bus::rcp.dpc.reg[DPC_CLOCK_REG];
    Info.DPC_BUFBUSY_REG = &Bus::rcp.dpc.reg[DPC_BUFBUSY_REG];
    Info.DPC_PIPEBUSY_REG = &Bus::rcp.dpc.reg[DPC_PIPEBUSY_REG];
    Info.DPC_TMEM_REG = &Bus::rcp.dpc.reg[DPC_TMEM_REG];

    Info.VI_STATUS_REG = &Bus::rcp.vi.reg[VI_STATUS_REG];
    Info.VI_ORIGIN_REG = &Bus::rcp.vi.reg[VI_ORIGIN_REG];
    Info.VI_WIDTH_REG = &Bus::rcp.vi.reg[VI_WIDTH_REG];
    Info.VI_INTR_REG = &Bus::rcp.vi.reg[VI_INTR_REG];
    Info.VI_V_CURRENT_LINE_REG = &Bus::rcp.vi.reg[VI_CURRENT_REG];
    Info.VI_TIMING_REG = &Bus::rcp.vi.reg[VI_BURST_REG];
    Info.VI_V_SYNC_REG = &Bus::rcp.vi.reg[VI_V_SYNC_REG];
    Info.VI_H_SYNC_REG = &Bus::rcp.vi.reg[VI_H_SYNC_REG];
    Info.VI_LEAP_REG = &Bus::rcp.vi.reg[VI_LEAP_REG];
    Info.VI_H_START_REG = &Bus::rcp.vi.reg[VI_H_START_REG];
    Info.VI_V_START_REG = &Bus::rcp.vi.reg[VI_V_START_REG];
    Info.VI_V_BURST_REG = &Bus::rcp.vi.reg[VI_V_BURST_REG];
    Info.VI_X_SCALE_REG = &Bus::rcp.vi.reg[VI_X_SCALE_REG];
    Info.VI_Y_SCALE_REG = &Bus::rcp.vi.reg[VI_Y_SCALE_REG];

    _initialized = (InitiateGFX(Info) != 0);

    return _initialized ? OP_OK : OP_ERROR;
}
