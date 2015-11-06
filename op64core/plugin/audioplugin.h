#pragma once

#include <thread>
#include <atomic>

#include "iplugin.h"

#include <rom/rom.h>

class AudioPlugin : public IPlugin
{
public:
    virtual ~AudioPlugin();

    virtual OPStatus initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar);
    static OPStatus loadPlugin(const char* libPath, AudioPlugin*& outplug);

    
    void DacrateChanged(SystemType Type);

    void(*LenChanged)(void);
    unsigned int(*ReadLength)(void);
    void(*ProcessAList)(void);

protected:
    virtual OPStatus unloadPlugin();

private:
    virtual int GetDefaultSettingStartRange() const { return FirstAudioDefaultSet; }
    virtual int GetSettingStartRange() const { return FirstAudioSettings; }

    AudioPlugin();

    // Not implemented
    AudioPlugin(const AudioPlugin&) = delete;
    AudioPlugin& operator=(const AudioPlugin&) = delete;

    static void audioThread(AudioPlugin* plug);

    void stopUpdateThread(void);
    void startUpdateThread(void);

private:
    static std::atomic_bool _audioThreadRun;
    std::thread _audioThread;
    bool _usingThread;

    void(*Update)(int Wait);
    void(*AiDacrateChanged)(int Type);
};
