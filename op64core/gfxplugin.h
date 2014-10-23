#pragma once
#include "plugins.h"
#include <string>
#include "util.h"

typedef struct
{
    uint32_t addr;
    uint32_t size;
    uint32_t width;
    uint32_t height;
} FrameBufferInfo;

class GfxPlugin
{
public:
    GfxPlugin(const char* libPath);
    ~GfxPlugin();

    bool initialize(void* renderWindow, void* statusBar);
    inline bool isInitialized(void) { return _initialized; }
    void close(void);
    void onRomOpen(void);
    void onRomClose(void);
    void GameReset(void);
    std::string PluginName(void) const { return _pluginInfo.Name; }

    void(*CaptureScreen)(const char *);
    void(*ChangeWindow)(void);
    void(*Config)(void* hParent);
    void(*DrawScreen)(void);
    void(*MoveScreen)(int32_t xpos, int32_t ypos);
    void(*ProcessDList)(void);
    void(*ProcessRDPList)(void);
    void(*ShowCFB)(void);
    void(*UpdateScreen)(void);
    void(*ViStatusChanged)(void);
    void(*ViWidthChanged)(void);
    void(*SoftReset)(void);

    // frame buffer extension
    void(*fbRead)(uint32_t addr);
    void(*fbWrite)(uint32_t addr, uint32_t size);
    void(*fbGetInfo)(void* p);

private:
    GfxPlugin();
    GfxPlugin(const GfxPlugin&);
    GfxPlugin& operator=(const GfxPlugin&);

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
