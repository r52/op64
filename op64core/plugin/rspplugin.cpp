#include "rspplugin.h"
#include "gfxplugin.h"
#include "audioplugin.h"

#include <plugin/plugincontainer.h>

#include <oplog.h>
#include <rcp/rcpcommon.h>
#include <rcp/rcp.h>
#include <core/bus.h>

RSPPlugin::RSPPlugin() :
    IPlugin(),
    DoRspCycles(nullptr),
    _cycleCount(0)
{
}

RSPPlugin::~RSPPlugin()
{
    closePlugin();
    unloadPlugin();
}

OPStatus RSPPlugin::initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar)
{
    //Get DLL information
    void (*GetDllInfo)(PLUGIN_INFO* PluginInfo);

    getPluginFunction(_libHandle, "GetDllInfo", GetDllInfo);
    if (GetDllInfo == nullptr)
    {
        LOG_ERROR(RSPPlugin) << "GetDllInfo not found";
        return OP_ERROR;
    }

    PLUGIN_INFO PluginInfo;
    GetDllInfo(&PluginInfo);

    if (PluginInfo.Version == 1 || PluginInfo.Version == 0x100) {
        LOG_ERROR(RSPPlugin) << "Unsupported plugin version";
        return OP_ERROR;
    }

    typedef struct {
        void* hInst;
        int MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                               bswap on a uint32_t (32 bits) boundry */
        uint8_t* RDRAM;
        uint8_t* DMEM;
        uint8_t* IMEM;

        uint32_t* MI__INTR_REG;

        uint32_t* SP__MEM_ADDR_REG;
        uint32_t* SP__DRAM_ADDR_REG;
        uint32_t* SP__RD_LEN_REG;
        uint32_t* SP__WR_LEN_REG;
        uint32_t* SP__STATUS_REG;
        uint32_t* SP__DMA_FULL_REG;
        uint32_t* SP__DMA_BUSY_REG;
        uint32_t* SP__PC_REG;
        uint32_t* SP__SEMAPHORE_REG;

        uint32_t* DPC__START_REG;
        uint32_t* DPC__END_REG;
        uint32_t* DPC__CURRENT_REG;
        uint32_t* DPC__STATUS_REG;
        uint32_t* DPC__CLOCK_REG;
        uint32_t* DPC__BUFBUSY_REG;
        uint32_t* DPC__PIPEBUSY_REG;
        uint32_t* DPC__TMEM_REG;

        void (*CheckInterrupts)(void);
        void (*ProcessDlist)(void);
        void (*ProcessAlist)(void);
        void (*ProcessRdpList)(void);
        void (*ShowCFB)(void);
    } RSP_INFO_1_1;

    //Get Function from DLL
    void (*InitiateRSP)(RSP_INFO_1_1 Audio_Info, unsigned int* Cycles);
    getPluginFunction(_libHandle, "InitiateRSP", InitiateRSP);
    if (InitiateRSP == nullptr)
    {
        LOG_ERROR(RSPPlugin) << "InitiateRSP not found";
        return OP_ERROR;
    }

    RSP_INFO_1_1 Info;
    memset(&Info, 0, sizeof(Info));

    //Send Initialization information to the DLL
    Info.CheckInterrupts = DummyFunction;
    Info.ProcessDlist = plugins->gfx()->ProcessDList;
    Info.ProcessRdpList = plugins->gfx()->ProcessRDPList;
    Info.ShowCFB = plugins->gfx()->ShowCFB;
    Info.ProcessAlist = plugins->audio()->ProcessAList;

    Info.hInst = opLibGetMainHandle();
    Info.RDRAM = (uint8_t*)Bus::rdram.mem;
    Info.DMEM = (uint8_t*)Bus::rcp.sp.dmem;
    Info.IMEM = (uint8_t*)Bus::rcp.sp.imem;
    Info.MemoryBswaped = 0;

    Info.MI__INTR_REG = &Bus::rcp.mi.reg[MI_INTR_REG];

    Info.SP__MEM_ADDR_REG = &Bus::rcp.sp.reg[SP_MEM_ADDR_REG];
    Info.SP__DRAM_ADDR_REG = &Bus::rcp.sp.reg[SP_DRAM_ADDR_REG];
    Info.SP__RD_LEN_REG = &Bus::rcp.sp.reg[SP_RD_LEN_REG];
    Info.SP__WR_LEN_REG = &Bus::rcp.sp.reg[SP_WR_LEN_REG];
    Info.SP__STATUS_REG = &Bus::rcp.sp.reg[SP_STATUS_REG];
    Info.SP__DMA_FULL_REG = &Bus::rcp.sp.reg[SP_DMA_FULL_REG];
    Info.SP__DMA_BUSY_REG = &Bus::rcp.sp.reg[SP_DMA_BUSY_REG];
    Info.SP__PC_REG = &Bus::rcp.sp.stat[SP_PC_REG];
    Info.SP__SEMAPHORE_REG = &Bus::rcp.sp.reg[SP_SEMAPHORE_REG];

    Info.DPC__START_REG = &Bus::rcp.dpc.reg[DPC_START_REG];
    Info.DPC__END_REG = &Bus::rcp.dpc.reg[DPC_END_REG];
    Info.DPC__CURRENT_REG = &Bus::rcp.dpc.reg[DPC_CURRENT_REG];
    Info.DPC__STATUS_REG = &Bus::rcp.dpc.reg[DPC_STATUS_REG];
    Info.DPC__CLOCK_REG = &Bus::rcp.dpc.reg[DPC_CLOCK_REG];
    Info.DPC__BUFBUSY_REG = &Bus::rcp.dpc.reg[DPC_BUFBUSY_REG];
    Info.DPC__PIPEBUSY_REG = &Bus::rcp.dpc.reg[DPC_PIPEBUSY_REG];
    Info.DPC__TMEM_REG = &Bus::rcp.dpc.reg[DPC_TMEM_REG];

    InitiateRSP(Info, &_cycleCount);
    _initialized = true;

    return OP_OK;
}

OPStatus RSPPlugin::loadPlugin(const char* libPath, RSPPlugin*& outplug)
{
    if (nullptr != outplug)
    {
        LOG_DEBUG(RSPPlugin) << "Existing Plugin object";
        return OP_ERROR;
    }

    RSPPlugin* plugin = new RSPPlugin;

    if (OP_ERROR == loadLibrary(libPath, plugin->_libHandle, plugin->_pluginInfo))
    {
        delete plugin;
        LOG_ERROR(RSPPlugin) << "Error loading library " << libPath;
        return OP_ERROR;
    }

    //Find entries for functions in DLL
    void(*InitFunc)(void);

    plugin->loadDefaults();
    plugin->loadPJ64Settings();

    getPluginFunction(plugin->_libHandle, "DoRspCycles", plugin->DoRspCycles);
    getPluginFunction(plugin->_libHandle, "InitiateRSP", InitFunc);

    //version 102 functions
    getPluginFunction(plugin->_libHandle, "PluginLoaded", plugin->PluginOpened);

    //Make sure dll had all needed functions
    if (plugin->DoRspCycles == nullptr || 
        InitFunc == nullptr ||
        plugin->RomClosed == nullptr ||
        plugin->CloseLib == nullptr)
    { 
        freeLibrary(plugin->_libHandle);
        delete plugin;
        LOG_ERROR(RSPPlugin) << "Invalid plugin: not all required functions are found";
        return OP_ERROR;
    }

    if (plugin->_pluginInfo.Version >= 0x0102)
    {
        if (plugin->PluginOpened == nullptr)
        { 
            freeLibrary(plugin->_libHandle);
            delete plugin;
            LOG_ERROR(RSPPlugin) << "Invalid plugin: not all required functions are found";
            return OP_ERROR;
        }

        plugin->PluginOpened();
    }

    // Return it
    outplug = plugin; plugin = nullptr;

    return OP_OK;
}

OPStatus RSPPlugin::unloadPlugin()
{
    if (OP_ERROR == freeLibrary(_libHandle))
    {
        LOG_FATAL(RSPPlugin) << "Error unloading plugin";
        return OP_ERROR;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    DoRspCycles = nullptr;
    RomClosed = nullptr;
    Config = nullptr;
    CloseLib = nullptr;
    PluginOpened = nullptr;

    return OP_OK;
}

