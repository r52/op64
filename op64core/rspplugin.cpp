#include "rspplugin.h"
#include "gfxplugin.h"
#include "audioplugin.h"
#include "rcpcommon.h"
#include "rcp.h"
#include "bus.h"

RSPPlugin::RSPPlugin(const char* libPath) :
Config(false),
DoRspCycles(false),
CloseDLL(false),
RomOpen(false),
RomClosed(false),
PluginOpened(false),
_libHandle(false),
_initialized(false),
_romOpened(false),
_cycleCount(0)
{
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    loadLibrary(libPath);
}

RSPPlugin::~RSPPlugin()
{
    close();
    unloadLibrary();
}

bool RSPPlugin::initialize(Plugins* plugins, void* renderWindow, void* statusBar)
{
    //Get DLL information
    void (*GetDllInfo)(PLUGIN_INFO* PluginInfo);
    GetDllInfo = (void(*)(PLUGIN_INFO*))opLibGetFunc(_libHandle, "GetDllInfo");
    if (GetDllInfo == nullptr) { return false; }

    PLUGIN_INFO PluginInfo;
    GetDllInfo(&PluginInfo);

    if (PluginInfo.Version == 1 || PluginInfo.Version == 0x100) {
        return false;
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
    InitiateRSP = (void(*)(RSP_INFO_1_1, unsigned int*))opLibGetFunc(_libHandle, "InitiateRSP");
    if (InitiateRSP == nullptr) { return false; }

    RSP_INFO_1_1 Info;
    memset(&Info, 0, sizeof(Info));

    //Send Initilization information to the DLL
    Info.CheckInterrupts = DummyFunction;
    Info.ProcessDlist = plugins->gfx()->ProcessDList;
    Info.ProcessRdpList = plugins->gfx()->ProcessRDPList;
    Info.ShowCFB = plugins->gfx()->ShowCFB;
    Info.ProcessAlist = plugins->audio()->ProcessAList;

    Info.hInst = GetModuleHandle(NULL);
    Info.RDRAM = (uint8_t*)Bus::rdram->mem;
    Info.DMEM = (uint8_t*)Bus::rcp->sp.dmem;
    Info.IMEM = (uint8_t*)Bus::rcp->sp.imem;
    Info.MemoryBswaped = FALSE;

    Info.MI__INTR_REG = &Bus::rcp->mi.reg[MI_INTR_REG];

    Info.SP__MEM_ADDR_REG = &Bus::rcp->sp.reg[SP_MEM_ADDR_REG];
    Info.SP__DRAM_ADDR_REG = &Bus::rcp->sp.reg[SP_DRAM_ADDR_REG];
    Info.SP__RD_LEN_REG = &Bus::rcp->sp.reg[SP_RD_LEN_REG];
    Info.SP__WR_LEN_REG = &Bus::rcp->sp.reg[SP_WR_LEN_REG];
    Info.SP__STATUS_REG = &Bus::rcp->sp.reg[SP_STATUS_REG];
    Info.SP__DMA_FULL_REG = &Bus::rcp->sp.reg[SP_DMA_FULL_REG];
    Info.SP__DMA_BUSY_REG = &Bus::rcp->sp.reg[SP_DMA_BUSY_REG];
    Info.SP__PC_REG = &Bus::rcp->sp.stat[SP_PC_REG];
    Info.SP__SEMAPHORE_REG = &Bus::rcp->sp.reg[SP_SEMAPHORE_REG];

    Info.DPC__START_REG = &Bus::rcp->dpc.reg[DPC_START_REG];
    Info.DPC__END_REG = &Bus::rcp->dpc.reg[DPC_END_REG];
    Info.DPC__CURRENT_REG = &Bus::rcp->dpc.reg[DPC_CURRENT_REG];
    Info.DPC__STATUS_REG = &Bus::rcp->dpc.reg[DPC_STATUS_REG];
    Info.DPC__CLOCK_REG = &Bus::rcp->dpc.reg[DPC_CLOCK_REG];
    Info.DPC__BUFBUSY_REG = &Bus::rcp->dpc.reg[DPC_BUFBUSY_REG];
    Info.DPC__PIPEBUSY_REG = &Bus::rcp->dpc.reg[DPC_PIPEBUSY_REG];
    Info.DPC__TMEM_REG = &Bus::rcp->dpc.reg[DPC_TMEM_REG];

    InitiateRSP(Info, &_cycleCount);
    _initialized = true;

    return _initialized;
}

void RSPPlugin::close(void)
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

void RSPPlugin::onRomOpen(void)
{
    if (!_romOpened)
    {
        if (RomOpen)
        {
            RomOpen();
        }
        _romOpened = true;
    }
}

void RSPPlugin::onRomClose(void)
{
    if (_romOpened)
    {
        RomClosed();
        _romOpened = false;
    }
}

void RSPPlugin::GameReset(void)
{
    if (_romOpened)
    {
        onRomClose();
        onRomOpen();
    }
}

void RSPPlugin::loadLibrary(const char* libPath)
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

    //Find entries for functions in DLL
    void (*InitFunc)(void);
    DoRspCycles = (unsigned int(*)(unsigned int))opLibGetFunc(_libHandle, "DoRspCycles");
    InitFunc = (void(*)(void))  opLibGetFunc(_libHandle, "InitiateRSP");
    RomClosed = (void(*)(void))  opLibGetFunc(_libHandle, "RomClosed");
    RomOpen = (void(*)(void))  opLibGetFunc(_libHandle, "RomOpen");
    CloseDLL = (void(*)(void))  opLibGetFunc(_libHandle, "CloseDLL");
    Config = (void(*)(void*)) opLibGetFunc(_libHandle, "DllConfig");

    //version 102 functions
    PluginOpened = (void(*)(void))opLibGetFunc(_libHandle, "PluginLoaded");

    //Make sure dll had all needed functions
    if (DoRspCycles == nullptr) { unloadLibrary(); return; }
    if (InitFunc == nullptr) { unloadLibrary(); return; }
    if (RomClosed == nullptr) { unloadLibrary(); return; }
    if (CloseDLL == nullptr) { unloadLibrary(); return; }

    if (_pluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == nullptr) { unloadLibrary(); return; }

        PluginOpened();
    }
}

void RSPPlugin::unloadLibrary(void)
{
    if (nullptr != _libHandle) {
        opLibClose(_libHandle);
        _libHandle = nullptr;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    DoRspCycles = nullptr;
    RomClosed = nullptr;
    Config = nullptr;
    CloseDLL = nullptr;
    PluginOpened = nullptr;
}
