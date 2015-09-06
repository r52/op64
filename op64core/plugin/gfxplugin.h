#pragma once

#include "iplugin.h"


class GfxPlugin : public IPlugin
{
public:
    virtual ~GfxPlugin();

    virtual OPStatus initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar);
    static OPStatus loadPlugin(const char* libPath, GfxPlugin*& outplug);

    void(*CaptureScreen)(const char *);
    void(*ChangeWindow)(void);
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

protected:
    virtual OPStatus unloadPlugin();

private:
    GfxPlugin();

    // Not implemented
    GfxPlugin(const GfxPlugin&);
    GfxPlugin& operator=(const GfxPlugin&);
};
