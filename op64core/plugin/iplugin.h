#pragma once

#include <string>
#include <oplib.h>

#include "plugintypes.h"
#include "pj64settingwrapper.h"

class Bus;
class PluginContainer;

class IPlugin
{
public:
    virtual ~IPlugin();
    virtual OPStatus initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar) = 0;
    virtual inline bool isInitialized(void) final { return _initialized; }
    virtual void closePlugin(void) final;
    virtual void onRomOpen(void) final;
    virtual void onRomClose(void) final;
    virtual void gameReset(void) final;
    virtual std::string pluginName(void) const final { return _pluginInfo.Name; }

    virtual int GetDefaultSettingStartRange() const = 0;
    virtual int GetSettingStartRange() const = 0;

    void(*Config)(void* hParent);

protected:
    IPlugin();
    static bool validPluginVersion(PLUGIN_INFO& pluginInfo);
    static OPStatus loadLibrary(const char* libPath, LibHandle& handle, PLUGIN_INFO& pluginInfo);
    static OPStatus freeLibrary(LibHandle& handle);

    virtual OPStatus unloadPlugin() = 0;

    // From pj64
    template <typename T>
    static inline void getPluginFunction(LibHandle& handle, const char * funcname, T & funcptr) {
        funcptr = (T) opLibGetFunc(handle, funcname);
    }

    void loadDefaults();
    void loadPJ64Settings();

    void(*SetSettingInfo)(PLUGIN_SETTINGS*);
    void(*SetSettingInfo2)(PLUGIN_SETTINGS2*);

private:
    // Not implemented
    IPlugin(const IPlugin&);
    IPlugin& operator=(const IPlugin&);

protected:
    LibHandle _libHandle;
    bool _initialized;
    bool _romOpened;
    PLUGIN_INFO _pluginInfo;

    void(*CloseLib)(void);
    void(*RomOpen)(void);
    void(*RomClosed)(void);
    void(*PluginOpened)(void);
};

void DummyFunction(void);
