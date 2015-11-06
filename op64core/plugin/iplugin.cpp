#include "iplugin.h"

#include <oplog.h>
#include <boost/bimap.hpp>

#include <ui/configstore.h>

namespace PJ64Settings
{
    typedef boost::bimap< uint32_t, std::string > bm_type;
    bm_type PJSettingStore;

    uint32_t FindSetting(void* handle, const char* Name)
    {
        auto right_iter = PJSettingStore.right.find(Name);

        if (right_iter != PJSettingStore.right.end())
        {
            return right_iter->second;
        }

        return 0;
    }

    uint32_t GetSetting(void* handle, int Type)
    {
        auto left_iter = PJSettingStore.left.find((uint32_t)Type);
        if (left_iter != PJSettingStore.left.end())
        {
            return ConfigStore::getInstance().getInt("PJ64", left_iter->second);
        }

        return 0;
    }

    const char* GetSettingSz(void* handle, int Type, char* buf, int bufsize)
    {
        if (buf && bufsize > 0)
        {
            buf[0] = 0;

            auto left_iter = PJSettingStore.left.find((uint32_t)Type);
            if (left_iter != PJSettingStore.left.end())
            {
                std::string res = ConfigStore::getInstance().getString("PJ64", left_iter->second);
                strncpy_s(buf, bufsize, res.c_str(), res.length());
                buf[bufsize - 1] = '\0';
            }
        }

        return buf;
    }

    void SetSetting(void* handle, int ID, unsigned int Value)
    {
        auto left_iter = PJSettingStore.left.find((uint32_t)ID);
        if (left_iter != PJSettingStore.left.end())
        {
            return ConfigStore::getInstance().set("PJ64", left_iter->second, Value);
        }
    }

    void SetSettingSz(void* handle, int ID, const char * Value)
    {
        auto left_iter = PJSettingStore.left.find((uint32_t)ID);
        if (left_iter != PJSettingStore.left.end())
        {
            return ConfigStore::getInstance().set("PJ64", left_iter->second, Value);
        }
    }

    void RegisterSetting(void* handle, int ID, int DefaultID, SettingDataType DataType,
        SettingType Type, const char * Category, const char * DefaultStr,
        uint32_t Value)
    {
        std::string cfgp;

        if (0 == strlen(DefaultStr))
        {
            cfgp = std::string(Category) + "-" + std::to_string(ID);
        }
        else
        {
            cfgp = std::string(Category) + "-" + DefaultStr;
        }

        PJSettingStore.insert(bm_type::value_type((uint32_t)ID, cfgp));

        if (!ConfigStore::getInstance().exists("PJ64", cfgp))
        {
            switch (DataType)
            {
            case Data_DWORD:
                if (DefaultID == 0)
                {
                    ConfigStore::getInstance().set("PJ64", cfgp, Value);
                }
                else
                {
                    ConfigStore::getInstance().set("PJ64", cfgp, DefaultID);
                }
                break;
            case Data_String:
                if (DefaultID == 0)
                {
                    ConfigStore::getInstance().set("PJ64", cfgp, "");
                }
                else {
                    ConfigStore::getInstance().set("PJ64", cfgp, DefaultID);
                }
                break;
            default:
                LOG_FATAL(PJ64Settings) << "Invalid data type " << DataType;
            }
        }
    }
}

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

void IPlugin::loadDefaults()
{
    getPluginFunction(_libHandle, "RomClosed", RomClosed);
    getPluginFunction(_libHandle, "RomOpen", RomOpen);
    getPluginFunction(_libHandle, "CloseDLL", CloseLib);
    getPluginFunction(_libHandle, "DllConfig", Config);
}

void IPlugin::loadPJ64Settings()
{
    getPluginFunction(_libHandle, "SetSettingInfo2", SetSettingInfo2);
    if (SetSettingInfo2)
    {
        PLUGIN_SETTINGS2 info;
        info.FindSystemSettingId = (unsigned int(*)(void * handle, const char *))PJ64Settings::FindSetting;
        SetSettingInfo2(&info);
    }

    getPluginFunction(_libHandle, "SetSettingInfo", SetSettingInfo);
    if (SetSettingInfo)
    {
        PLUGIN_SETTINGS info;
        info.dwSize = sizeof(PLUGIN_SETTINGS);
        info.DefaultStartRange = GetDefaultSettingStartRange();
        info.SettingStartRange = GetSettingStartRange();
        info.MaximumSettings = MaxPluginSetting;
        info.NoDefault = 0;
        info.DefaultLocation = SettingType_CfgFile;
        info.handle = opLibGetMainHandle();
        info.RegisterSetting = (void(*)(void *, int, int, SettingDataType, SettingType, const char *, const char *, DWORD))&PJ64Settings::RegisterSetting;
        info.GetSetting = (unsigned int(*)(void *, int))&PJ64Settings::GetSetting;
        info.GetSettingSz = (const char * (*)(void *, int, char *, int))&PJ64Settings::GetSettingSz;
        info.SetSetting = (void(*)(void *, int, unsigned int))&PJ64Settings::SetSetting;
        info.SetSettingSz = (void(*)(void *, int, const char *))&PJ64Settings::SetSettingSz;
        info.UseUnregisteredSetting = NULL;

        SetSettingInfo(&info);
    }
}

void DummyFunction(void)
{
}
