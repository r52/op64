#include "rspplugin.h"
#include "gfxplugin.h"
#include "audioplugin.h"
#include "rcpcommon.h"

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
    void(*GetDllInfo) (PLUGIN_INFO * PluginInfo);
    GetDllInfo = (void(*)(PLUGIN_INFO *))opLibGetFunc(_libHandle, "GetDllInfo");
    if (GetDllInfo == nullptr) { return false; }

    PLUGIN_INFO PluginInfo;
    GetDllInfo(&PluginInfo);

    if (PluginInfo.Version == 1 || PluginInfo.Version == 0x100) {
        return false;
    }

    typedef struct {
        HINSTANCE hInst;
        BOOL MemoryBswaped;    /* If this is set to TRUE, then the memory has been pre
                               bswap on a dword (32 bits) boundry */
        BYTE * RDRAM;
        BYTE * DMEM;
        BYTE * IMEM;

        DWORD * MI__INTR_REG;

        DWORD * SP__MEM_ADDR_REG;
        DWORD * SP__DRAM_ADDR_REG;
        DWORD * SP__RD_LEN_REG;
        DWORD * SP__WR_LEN_REG;
        DWORD * SP__STATUS_REG;
        DWORD * SP__DMA_FULL_REG;
        DWORD * SP__DMA_BUSY_REG;
        DWORD * SP__PC_REG;
        DWORD * SP__SEMAPHORE_REG;

        DWORD * DPC__START_REG;
        DWORD * DPC__END_REG;
        DWORD * DPC__CURRENT_REG;
        DWORD * DPC__STATUS_REG;
        DWORD * DPC__CLOCK_REG;
        DWORD * DPC__BUFBUSY_REG;
        DWORD * DPC__PIPEBUSY_REG;
        DWORD * DPC__TMEM_REG;

        void(*CheckInterrupts)(void);
        void(*ProcessDlist)(void);
        void(*ProcessAlist)(void);
        void(*ProcessRdpList)(void);
        void(*ShowCFB)(void);
    } RSP_INFO_1_1;

    //Get Function from DLL
    void(*InitiateRSP)    (RSP_INFO_1_1 Audio_Info, unsigned int* Cycles);
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
    Info.RDRAM = Bus::rdram8;
    Info.DMEM = Bus::sp_dmem8;
    Info.IMEM = Bus::sp_imem8;
    Info.MemoryBswaped = FALSE;

    Info.MI__INTR_REG = (DWORD*)&Bus::mi_reg[MI_INTR_REG];

    Info.SP__MEM_ADDR_REG = (DWORD*)&Bus::sp_reg[SP_MEM_ADDR_REG];
    Info.SP__DRAM_ADDR_REG = (DWORD*)&Bus::sp_reg[SP_DRAM_ADDR_REG];
    Info.SP__RD_LEN_REG = (DWORD*)&Bus::sp_reg[SP_RD_LEN_REG];
    Info.SP__WR_LEN_REG = (DWORD*)&Bus::sp_reg[SP_WR_LEN_REG];
    Info.SP__STATUS_REG = (DWORD*)&Bus::sp_reg[SP_STATUS_REG];
    Info.SP__DMA_FULL_REG = (DWORD*)&Bus::sp_reg[SP_DMA_FULL_REG];
    Info.SP__DMA_BUSY_REG = (DWORD*)&Bus::sp_reg[SP_DMA_BUSY_REG];
    Info.SP__PC_REG = (DWORD*)&Bus::sp_reg[SP_PC_REG];
    Info.SP__SEMAPHORE_REG = (DWORD*)&Bus::sp_reg[SP_SEMAPHORE_REG];

    Info.DPC__START_REG = (DWORD*)&Bus::dp_reg[DPC_START_REG];
    Info.DPC__END_REG = (DWORD*)&Bus::dp_reg[DPC_END_REG];
    Info.DPC__CURRENT_REG = (DWORD*)&Bus::dp_reg[DPC_CURRENT_REG];
    Info.DPC__STATUS_REG = (DWORD*)&Bus::dp_reg[DPC_STATUS_REG];
    Info.DPC__CLOCK_REG = (DWORD*)&Bus::dp_reg[DPC_CLOCK_REG];
    Info.DPC__BUFBUSY_REG = (DWORD*)&Bus::dp_reg[DPC_BUFBUSY_REG];
    Info.DPC__PIPEBUSY_REG = (DWORD*)&Bus::dp_reg[DPC_PIPEBUSY_REG];
    Info.DPC__TMEM_REG = (DWORD*)&Bus::dp_reg[DPC_TMEM_REG];

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
        unloadLibrary();
        return;
    }

    void(*GetDllInfo) (PLUGIN_INFO * PluginInfo);
    GetDllInfo = (void(*)(PLUGIN_INFO *))opLibGetFunc(_libHandle, "GetDllInfo");
    if (GetDllInfo == NULL) { unloadLibrary(); return; }

    GetDllInfo(&_pluginInfo);
    if (!Plugins::ValidPluginVersion(_pluginInfo))
    {
        unloadLibrary();
        return;
    }

    //Find entries for functions in DLL
    void(*InitFunc)(void);
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
