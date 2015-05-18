#pragma once
#include <string>

#include <ui/configstore.h>
#include <plugin/plugintypes.h>

class GfxPlugin;
class AudioPlugin;
class RSPPlugin;
class InputPlugin;

class PluginContainer
{
public:
    PluginContainer();
    ~PluginContainer();

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

private:
    // Not implemented
    PluginContainer(const PluginContainer&);
    PluginContainer& operator=(const PluginContainer&);

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
