#pragma once

#include "iplugin.h"


class RSPPlugin : public IPlugin
{
public:
    virtual ~RSPPlugin();

    virtual OPStatus initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar);
    static OPStatus loadPlugin(const char* libPath, RSPPlugin*& outplug);

    unsigned int(*DoRspCycles)(unsigned int);

protected:
    virtual OPStatus unloadPlugin();

private:
    virtual int GetDefaultSettingStartRange() const { return FirstRSPDefaultSet; }
    virtual int GetSettingStartRange() const { return FirstRSPSettings; }

    RSPPlugin();

    // Not implemented
    RSPPlugin(const RSPPlugin&);
    RSPPlugin& operator=(const RSPPlugin&);

private:
    uint32_t _cycleCount;
};
