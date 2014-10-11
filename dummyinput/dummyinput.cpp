#include <cstdint>
#include <string.h>

#include "Controller #1.1.h"


EXPORT void CALL CloseDLL(void)
{

}

EXPORT void CALL ControllerCommand(int Control, uint8_t* Command)
{

}

EXPORT void CALL DllAbout(void* hParent)
{

}

EXPORT void CALL DllConfig(void* hParent)
{

}

EXPORT void CALL DllTest(void* hParent)
{

}

EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo)
{
    PluginInfo->Version = 0x0101;
    PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
    strcpy(PluginInfo->Name, "Dummy Input");
    return;
}

EXPORT void CALL GetKeys(int Control, BUTTONS* Keys)
{
    Keys->Value = 0x0000;
}

EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo)
{
    ControlInfo.Controls[0].Present = 1;
}

EXPORT void CALL ReadController(int Control, uint8_t* Command)
{

}

EXPORT void CALL RomClosed(void)
{

}

EXPORT void CALL RomOpen(void)
{

}

