#pragma once

#include "iplugin.h"
#include <core/inputtypes.h>

class InputPlugin : public IPlugin
{
public:
    virtual ~InputPlugin();

    virtual OPStatus initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar);
    static OPStatus loadPlugin(const char* libPath, InputPlugin*& outplug);

    void(*RumbleCommand)(int Control, int bRumble);
    void(*GetKeys)(int Control, BUTTONS * Keys);
    void(*ReadController)(int Control, uint8_t* Command);
    void(*ControllerCommand)(int Control, uint8_t* Command);

protected:
    virtual OPStatus unloadPlugin();

private:
    virtual int GetDefaultSettingStartRange() const { return FirstCtrlDefaultSet; }
    virtual int GetSettingStartRange() const { return FirstCtrlSettings; }

    InputPlugin();

    // Not implemented
    InputPlugin(const InputPlugin&);
    InputPlugin& operator=(const InputPlugin&);
};
