#pragma once
#include <cstdint>
#include <string>

#include "configstore.h"

typedef struct {
    uint16_t Version;
    uint16_t Type;
    char Name[100];

    int32_t NormalMemory;
    int32_t MemoryBswaped;
} PLUGIN_INFO;

enum PLUGIN_TYPE {
    PLUGIN_TYPE_NONE = 0,
    PLUGIN_TYPE_RSP = 1,
    PLUGIN_TYPE_GFX = 2,
    PLUGIN_TYPE_AUDIO = 3,
    PLUGIN_TYPE_CONTROLLER = 4,
};

class GfxPlugin;
class AudioPlugin;
class RSPPlugin;
class InputPlugin;

class Plugins
{
public:
    Plugins();
    ~Plugins();

    bool initialize(void);
    void RomOpened(void);
    void RomClosed(void);
    void ConfigPlugin(void* parentWindow, PLUGIN_TYPE Type);
    void Reset(void);
    void GameReset(void);
    void StopEmulation(void);

    void setRenderWindow(void* windowHandle);
    void setStatusBar(void* statusBar);

    inline GfxPlugin* gfx(void) { return _gfx;  }
    inline AudioPlugin* audio(void) { return _audio; }
    inline RSPPlugin* rsp(void) { return _rsp; }
    inline InputPlugin* input(void) { return _input; }

    static bool ValidPluginVersion(PLUGIN_INFO & PluginInfo);

private:
    // Not implemented
    Plugins(const Plugins&);
    Plugins& operator=(const Plugins&);

    void loadPlugins(void);
    void pluginsChanged(void);

    void DestroyGfxPlugin(void);
    void DestroyAudioPlugin(void);
    void DestroyRspPlugin(void);
    void DestroyInputPlugin(void);
    
private:
    void* _renderWindow;
    void* _statusBar;
    CallbackID _plugChangeCallback;
    GfxPlugin* _gfx;
    AudioPlugin* _audio;
    RSPPlugin* _rsp;
    InputPlugin* _input;
    std::string _gfxPath;
    std::string _audioPath;
    std::string _rspPath;
    std::string _inputPath;
};

void DummyFunction(void);
