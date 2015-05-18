#pragma once

#include <thread>
#include <atomic>

#include "iplugin.h"

#include <rom/rom.h>

class AudioPlugin : public IPlugin
{
public:
    virtual ~AudioPlugin();

    virtual OPStatus initialize(PluginContainer* plugins, void* renderWindow, void* statusBar);
    static OPStatus loadPlugin(const char* libPath, AudioPlugin*& outplug);

    
    void DacrateChanged(SystemType Type);

    void(*LenChanged)(void);
    void(*Config)(void* hParent);
    unsigned int(*ReadLength)(void);
    void(*ProcessAList)(void);

protected:
    virtual OPStatus unloadPlugin();

private:
    AudioPlugin();

    // Not implemented
    AudioPlugin(const AudioPlugin&);
    AudioPlugin& operator=(const AudioPlugin&);

    void audioThread(void);

private:
    std::atomic_bool _audioThreadStop;
    std::thread _audioThread;
    bool _usingThread;

    void(*Update)(int Wait);
    void(*AiDacrateChanged)(SystemType Type);
};
