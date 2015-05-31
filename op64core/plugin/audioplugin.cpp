#include "audioplugin.h"

#include <core/bus.h>
#include <rcp/rcp.h>

std::atomic_bool AudioPlugin::_audioThreadRun{ false };

AudioPlugin::AudioPlugin() :
    _usingThread(false),
    LenChanged(nullptr),
    ReadLength(nullptr),
    ProcessAList(nullptr),
    Update(nullptr),
    AiDacrateChanged(nullptr)
{
}

AudioPlugin::~AudioPlugin()
{
    closePlugin();
    unloadPlugin();
}

OPStatus AudioPlugin::initialize(PluginContainer* plugins, void* renderWindow, void* statusBar)
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

    int(*InitiateAudio)(AUDIO_INFO Audio_Info);

    getPluginFunction(_libHandle, "InitiateAudio", InitiateAudio);
    if (InitiateAudio == nullptr)
    {
        LOG_ERROR(AudioPlugin) << "InitiateAudio not found";
        return OP_ERROR;
    }

    AUDIO_INFO Info;
    memset(&Info, 0, sizeof(Info));

    Info.hwnd = renderWindow;

    Info.hinst = opLibGetMainHandle();
    Info.MemoryBswaped = 1;
    if (Bus::rom)
    {
        Info.HEADER = Bus::rom->getImage();
    }
    else
    {
        uint8_t buf[100];
        Info.HEADER = buf;
    }
    Info.RDRAM = (uint8_t*)Bus::rdram->mem;
    Info.DMEM = (uint8_t*)Bus::rcp->sp.dmem;
    Info.IMEM = (uint8_t*)Bus::rcp->sp.imem;
    Info.MI__INTR_REG = &Bus::rcp->mi.reg[MI_INTR_REG];
    Info.AI__DRAM_ADDR_REG = &Bus::rcp->ai.reg[AI_DRAM_ADDR_REG];
    Info.AI__LEN_REG = &Bus::rcp->ai.reg[AI_LEN_REG];
    Info.AI__CONTROL_REG = &Bus::rcp->ai.reg[AI_CONTROL_REG];
    Info.AI__STATUS_REG = &Bus::rcp->ai.reg[AI_STATUS_REG];
    Info.AI__DACRATE_REG = &Bus::rcp->ai.reg[AI_DACRATE_REG];
    Info.AI__BITRATE_REG = &Bus::rcp->ai.reg[AI_BITRATE_REG];
    Info.CheckInterrupts = DummyFunction;

    _initialized = InitiateAudio(Info) != 0;

    if (Update) {
        startUpdateThread();
    }

    if (Bus::rcp->ai.reg[AI_DACRATE_REG] != 0) {
        DacrateChanged(Bus::rom ? Bus::rom->getSystemType() : SYSTEM_NTSC);
    }

    return _initialized ? OP_OK : OP_ERROR;
}

OPStatus AudioPlugin::loadPlugin(const char* libPath, AudioPlugin*& outplug)
{
    if (nullptr != outplug)
    {
        LOG_DEBUG(AudioPlugin) << "Existing Plugin object";
        return OP_ERROR;
    }

    AudioPlugin* plugin = new AudioPlugin;

    if (OP_ERROR == loadLibrary(libPath, plugin->_libHandle, plugin->_pluginInfo))
    {
        delete plugin;
        LOG_ERROR(AudioPlugin) << "Error loading library " << libPath;
        return OP_ERROR;
    }

    //Find entries for functions in DLL
    void(*InitFunc)(void);

    getPluginFunction(plugin->_libHandle, "RomClosed", plugin->RomClosed);
    getPluginFunction(plugin->_libHandle, "RomOpen", plugin->RomOpen);
    getPluginFunction(plugin->_libHandle, "CloseDLL", plugin->CloseLib);
    getPluginFunction(plugin->_libHandle, "DllConfig", plugin->Config);

    getPluginFunction(plugin->_libHandle, "AiDacrateChanged", plugin->AiDacrateChanged);
    getPluginFunction(plugin->_libHandle, "InitiateAudio", InitFunc);
    getPluginFunction(plugin->_libHandle, "AiLenChanged", plugin->LenChanged);
    getPluginFunction(plugin->_libHandle, "AiReadLength", plugin->ReadLength);
    getPluginFunction(plugin->_libHandle, "ProcessAList", plugin->ProcessAList);

    getPluginFunction(plugin->_libHandle, "AiUpdate", plugin->Update);

    //version 102 functions
    getPluginFunction(plugin->_libHandle, "PluginLoaded", plugin->PluginOpened);

    if (plugin->AiDacrateChanged == nullptr ||
        plugin->LenChanged == nullptr ||
        plugin->ReadLength == nullptr ||
        InitFunc == nullptr ||
        plugin->RomClosed == nullptr ||
        plugin->ProcessAList == nullptr)
    {
        freeLibrary(plugin->_libHandle);
        delete plugin;
        LOG_ERROR(AudioPlugin) << "Invalid plugin: not all required functions are found";
        return OP_ERROR;
    }

    if (plugin->_pluginInfo.Version >= 0x0102)
    {
        if (plugin->PluginOpened == nullptr)
        {
            freeLibrary(plugin->_libHandle);
            delete plugin;
            LOG_ERROR(AudioPlugin) << "Invalid plugin: not all required functions are found";
            return OP_ERROR;
        }

        plugin->PluginOpened();
    }

    // Return it
    outplug = plugin; plugin = nullptr;

    return OP_OK;
}

void AudioPlugin::DacrateChanged(SystemType Type)
{
    if (!isInitialized()) { return; }

    AiDacrateChanged(static_cast<int>(Type));
}

OPStatus AudioPlugin::unloadPlugin()
{
    stopUpdateThread();

    if (OP_ERROR == freeLibrary(_libHandle))
    {
        LOG_FATAL(AudioPlugin) << "Error unloading plugin";
        return OP_ERROR;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    AiDacrateChanged = nullptr;
    LenChanged = nullptr;
    Config = nullptr;
    ReadLength = nullptr;
    Update = nullptr;
    ProcessAList = nullptr;
    RomClosed = nullptr;
    CloseLib = nullptr;

    return OP_OK;
}

void AudioPlugin::audioThread(AudioPlugin* plug)
{
    while (_audioThreadRun)
    {
        plug->Update(1);
    }
}

void AudioPlugin::stopUpdateThread(void)
{
    if (_usingThread)
    {
        _audioThreadRun = false;
#ifdef _MSC_VER
        // Force pre-empt WaitMessage calls for azimer 0.55
        PostThreadMessage(GetThreadId(_audioThread.native_handle()), WM_QUIT, 0, 0);
#endif
        _audioThread.join();
        _usingThread = false;
    }
}

void AudioPlugin::startUpdateThread(void)
{
    if (!_usingThread)
    {
        _usingThread = true;
        _audioThreadRun = true;
        _audioThread = std::thread(AudioPlugin::audioThread, this);
    }
}
