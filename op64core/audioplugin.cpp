#include "audioplugin.h"
#include "rcpcommon.h"
#include "ai_controller.h"


AudioPlugin::AudioPlugin(const char* libPath) :
_libHandle(nullptr),
_initialized(false),
_romOpened(false),
_usingThread(false),
m_DacrateChanged(nullptr),
LenChanged(nullptr),
Config(nullptr),
ReadLength(nullptr),
RomOpen(nullptr),
RomClosed(nullptr),
CloseDLL(nullptr),
ProcessAList(nullptr),
Update(nullptr),
PluginOpened(nullptr)
{
    _audioThreadStop = false;
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    loadLibrary(libPath);
}

AudioPlugin::~AudioPlugin()
{
    close();
    unloadLibrary();
}

bool AudioPlugin::initialize(void* renderWindow, void* statusBar)
{
    // TODO future: linux version (mupen spec)
    struct AUDIO_INFO {
        void* hwnd;
        void* hinst;
        int MemoryBswaped;
        uint8_t* HEADER;
        uint8_t* RDRAM;
        uint8_t* DMEM;
        uint8_t* IMEM;

        uint32_t* MI__INTR_REG;

        uint32_t* AI__DRAM_ADDR_REG;
        uint32_t* AI__LEN_REG;
        uint32_t* AI__CONTROL_REG;
        uint32_t* AI__STATUS_REG;
        uint32_t* AI__DACRATE_REG;
        uint32_t* AI__BITRATE_REG;

        void(*CheckInterrupts)(void);
    };

    int (*InitiateAudio)(AUDIO_INFO Audio_Info);
    InitiateAudio = (int(*)(AUDIO_INFO))opLibGetFunc(_libHandle, "InitiateAudio");
    if (InitiateAudio == nullptr) { return false; }

    AUDIO_INFO Info;
    memset(&Info, 0, sizeof(Info));

    Info.hwnd = renderWindow;
    Info.hinst = GetModuleHandle(NULL);
    Info.MemoryBswaped = TRUE;
    Info.HEADER = Bus::rom_image;
    Info.RDRAM = Bus::rdram8;
    Info.DMEM = Bus::sp_dmem8;
    Info.IMEM = Bus::sp_imem8;
    Info.MI__INTR_REG = &Bus::mi_reg[MI_INTR_REG];
    Info.AI__DRAM_ADDR_REG = &Bus::ai.reg[AI_DRAM_ADDR_REG];
    Info.AI__LEN_REG = &Bus::ai.reg[AI_LEN_REG];
    Info.AI__CONTROL_REG = &Bus::ai.reg[AI_CONTROL_REG];
    Info.AI__STATUS_REG = &Bus::ai.reg[AI_STATUS_REG];
    Info.AI__DACRATE_REG = &Bus::ai.reg[AI_DACRATE_REG];
    Info.AI__BITRATE_REG = &Bus::ai.reg[AI_BITRATE_REG];
    Info.CheckInterrupts = DummyFunction;

    _initialized = InitiateAudio(Info) != 0;

    if (Update) {
        _usingThread = true;
        _audioThreadStop = false;
        _audioThread = std::thread(&AudioPlugin::audioThread, this);
        _audioThread.detach();
    }

    if (Bus::ai.reg[AI_DACRATE_REG] != 0) {
        DacrateChanged(SYSTEM_NTSC);
    }

    return _initialized;
}

void AudioPlugin::close(void)
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

void AudioPlugin::GameReset(void)
{
    if (_romOpened)
    {
        RomClosed();
        if (RomOpen)
        {
            RomOpen();
        }
    }
}

void AudioPlugin::onRomOpen(void)
{
    if (!_romOpened && RomOpen)
    {
        RomOpen();
        _romOpened = true;
    }
}

void AudioPlugin::onRomClose(void)
{
    if (_romOpened)
    {
        RomClosed();
        _romOpened = false;
    }
}

void AudioPlugin::DacrateChanged(SystemType Type)
{
    if (!isInitialized()) { return; }

    m_DacrateChanged(Type);
}

void AudioPlugin::loadLibrary(const char* libPath)
{
    unloadLibrary();

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

    void(*InitFunc)(void);
    m_DacrateChanged = (void(*)(SystemType))  opLibGetFunc(_libHandle, "AiDacrateChanged");
    LenChanged = (void(*)(void)) opLibGetFunc(_libHandle, "AiLenChanged");
    Config = (void(*)(void*))opLibGetFunc(_libHandle, "DllConfig");
    ReadLength = (unsigned int(*)(void))opLibGetFunc(_libHandle, "AiReadLength");
    InitFunc = (void(*)(void)) opLibGetFunc(_libHandle, "InitiateAudio");
    RomOpen = (void(*)(void)) opLibGetFunc(_libHandle, "RomOpen");
    RomClosed = (void(*)(void)) opLibGetFunc(_libHandle, "RomClosed");
    CloseDLL = (void(*)(void)) opLibGetFunc(_libHandle, "CloseDLL");
    ProcessAList = (void(*)(void)) opLibGetFunc(_libHandle, "ProcessAList");

    Update = (void(*)(int))opLibGetFunc(_libHandle, "AiUpdate");

    PluginOpened = (void(*)(void))opLibGetFunc(_libHandle, "PluginLoaded");

    if (m_DacrateChanged == nullptr) { unloadLibrary(); return; }
    if (LenChanged == nullptr) { unloadLibrary(); return; }
    if (ReadLength == nullptr) { unloadLibrary(); return; }
    if (InitFunc == nullptr) { unloadLibrary(); return; }
    if (RomClosed == nullptr) { unloadLibrary(); return; }
    if (ProcessAList == nullptr) { unloadLibrary(); return; }

    if (_pluginInfo.Version >= 0x0102)
    {
        if (PluginOpened == nullptr) { unloadLibrary(); return; }
        PluginOpened();
    }
}

void AudioPlugin::unloadLibrary(void)
{
    if (_usingThread)
    {
        _audioThreadStop = true;
        _usingThread = false;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));

    if (nullptr != _libHandle) {
        opLibClose(_libHandle);
        _libHandle = nullptr;
    }

    m_DacrateChanged = nullptr;
    LenChanged = nullptr;
    Config = nullptr;
    ReadLength = nullptr;
    Update = nullptr;
    ProcessAList = nullptr;
    RomClosed = nullptr;
    CloseDLL = nullptr;
}

void AudioPlugin::audioThread(void)
{
    while (!_audioThreadStop)
    {
        this->Update(true);
    }
}
