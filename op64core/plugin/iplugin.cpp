#include "iplugin.h"

#include <oplog.h>

IPlugin::~IPlugin()
{
    if (OP_ERROR == freeLibrary(_libHandle))
    {
        LOG_FATAL(IPlugin) << "Error unloading plugin";
    }
}

void IPlugin::closePlugin()
{
    if (_romOpened)
    {
        onRomClose();
    }

    if (_initialized)
    {
        CloseLib();
        _initialized = false;
    }
}

void IPlugin::onRomOpen()
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

void IPlugin::onRomClose()
{
    if (_romOpened)
    {
        RomClosed();
        _romOpened = false;
    }
}

void IPlugin::gameReset()
{
    if (_romOpened)
    {
        onRomClose();
        onRomOpen();
    }
}

IPlugin::IPlugin() :
    Config(nullptr),
    _libHandle(nullptr),
    _initialized(false),
    _romOpened(false),
    CloseLib(nullptr),
    RomOpen(nullptr),
    RomClosed(nullptr),
    PluginOpened(nullptr)
{
    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
}

bool IPlugin::validPluginVersion(PLUGIN_INFO& pluginInfo)
{
    switch (pluginInfo.Type)
    {
    case PLUGIN_TYPE_RSP:
        if (!pluginInfo.MemoryBswaped) { return false; }
        if (pluginInfo.Version == 0x0001) { return true; }
        if (pluginInfo.Version == 0x0100) { return true; }
        if (pluginInfo.Version == 0x0101) { return true; }
        if (pluginInfo.Version == 0x0102) { return true; }
        break;
    case PLUGIN_TYPE_GFX:
        if (!pluginInfo.MemoryBswaped) { return false; }
        if (pluginInfo.Version == 0x0102) { return true; }
        if (pluginInfo.Version == 0x0103) { return true; }
        if (pluginInfo.Version == 0x0104) { return true; }
        break;
    case PLUGIN_TYPE_AUDIO:
        if (!pluginInfo.MemoryBswaped) { return false; }
        if (pluginInfo.Version == 0x0101) { return true; }
        if (pluginInfo.Version == 0x0102) { return true; }
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (pluginInfo.Version == 0x0100) { return true; }
        if (pluginInfo.Version == 0x0101) { return true; }
        if (pluginInfo.Version == 0x0102) { return true; }
        break;
    }

    return false;
}

OPStatus IPlugin::loadLibrary(const char* libPath, LibHandle& handle, PLUGIN_INFO& pluginInfo)
{
    if (!opLoadLib(&handle, libPath))
    {
        LOG_ERROR(Plugin) << libPath << " failed to load";
        freeLibrary(handle);
        return OP_ERROR;
    }

    void(*GetPluginInfo)(PLUGIN_INFO* PluginInfo);
    GetPluginInfo = (void(*)(PLUGIN_INFO*))opLibGetFunc(handle, "GetDllInfo");
    if (GetPluginInfo == nullptr)
    {
        LOG_ERROR(Plugin) << libPath << ": invalid plugin";
        freeLibrary(handle);
        return OP_ERROR;
    }

    GetPluginInfo(&pluginInfo);
    if (!validPluginVersion(pluginInfo))
    {
        LOG_ERROR(Plugin) << libPath << ": unsupported plugin version";
        freeLibrary(handle);
        return OP_ERROR;
    }

    return OP_OK;
}

OPStatus IPlugin::freeLibrary(LibHandle& handle)
{
    if (nullptr != handle) {
         if (!opLibClose(handle))
         {
             LOG_FATAL(Plugin) << "Error freeing plugin!";
             return OP_ERROR;
         }
        handle = nullptr;
    }

    return OP_OK;
}

void DummyFunction(void)
{
}
