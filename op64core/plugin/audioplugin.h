#pragma once

#include <thread>
#include <atomic>

#include <library.h>

#include "plugins.h"

#include <rom/rom.h>

class AudioPlugin
{
public:
    AudioPlugin(const char* libPath);
    ~AudioPlugin();

    bool initialize(void* renderWindow, void* statusBar);
    inline bool isInitialized(void) { return _initialized; }
    void close(void);
    void GameReset(void);
    void onRomOpen(void);
    void onRomClose(void);
    std::string PluginName(void) const { return _pluginInfo.Name; }
    
    void DacrateChanged(SystemType Type);

    void(*LenChanged)(void);
    void(*Config)(void* hParent);
    unsigned int(*ReadLength)(void);
    void(*ProcessAList)(void);

private:
    AudioPlugin();
    AudioPlugin(const AudioPlugin&);
    AudioPlugin& operator=(const AudioPlugin&);

    void loadLibrary(const char* libPath);
    void unloadLibrary(void);

    void audioThread(void);

private:
    std::atomic_bool _audioThreadStop;
    std::thread _audioThread;
    LibHandle _libHandle;
    bool _initialized;
    bool _romOpened;
    bool _usingThread;
    PLUGIN_INFO _pluginInfo;

    void(*CloseDLL)(void);
    void(*RomOpen)(void);
    void(*RomClosed)(void);
    void(*Update)(int Wait);
    void(*PluginOpened)(void);
    void(*m_DacrateChanged)(SystemType Type);
};
