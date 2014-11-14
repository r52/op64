#pragma once

#include <string>

#include "plugins.h"
#include "library.h"


class RSPPlugin
{
public:
    RSPPlugin(const char* libPath);
    ~RSPPlugin();

    bool initialize(Plugins* plugins, void* renderWindow, void* statusBar);
    inline bool isInitialized(void) { return _initialized; }
    void close(void);
    void onRomOpen(void);
    void onRomClose(void);
    void GameReset(void);
    std::string PluginName(void) const { return _pluginInfo.Name; }

    void(*Config)(void* hParent);
    unsigned int(*DoRspCycles)(unsigned int);

private:
    RSPPlugin();
    RSPPlugin(const RSPPlugin&);
    RSPPlugin& operator=(const RSPPlugin&);

    void loadLibrary(const char* libPath);
    void unloadLibrary(void);

private:
    LibHandle _libHandle;
    bool _initialized;
    bool _romOpened;
    uint32_t _cycleCount;
    PLUGIN_INFO _pluginInfo;

    void(*CloseDLL)(void);
    void(*RomOpen)(void);
    void(*RomClosed)(void);
    void(*PluginOpened)(void);
};
