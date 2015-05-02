#pragma once
#include <string>
#include <cstdint>

#include <oplib.h>

#include "plugins.h"
#include <core/inputtypes.h>

class InputPlugin
{
public:
    InputPlugin(const char* libPath);
    ~InputPlugin();

    bool initialize(void* renderWindow, void* statusBar);
    inline bool isInitialized(void) { return _initialized; }
    //void UpdateKeys(void);
    void close(void);
    void onRomOpen(void);
    void onRomClose(void);
    void GameReset(void);
    std::string PluginName(void) const { return _pluginInfo.Name; }

    void(*Config)(void* hParent);
    void(*RumbleCommand)(int Control, int bRumble);
    void(*GetKeys)(int Control, BUTTONS * Keys);
    void(*ReadController)(int Control, uint8_t* Command);
    void(*ControllerCommand)(int Control, uint8_t* Command);

private:
    InputPlugin();
    InputPlugin(const InputPlugin&);
    InputPlugin& operator=(const InputPlugin&);

    void loadLibrary(const char* libPath);
    void unloadLibrary(void);

private:
    LibHandle _libHandle;
    bool _initialized;
    bool _romOpened;
    PLUGIN_INFO _pluginInfo;

    void(*CloseDLL)(void);
    void(*RomOpen)(void);
    void(*RomClosed)(void);
    void(*PluginOpened)(void);
};
