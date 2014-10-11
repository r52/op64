#include <windows.h>

#include "Audio #1.1.h"


EXPORT void CALL AiDacrateChanged(int SystemType)
{

}


EXPORT void CALL AiLenChanged(void)
{

}


EXPORT DWORD CALL AiReadLength(void)
{
    return 0;
}

/*
EXPORT void CALL AiUpdate(BOOL Wait)
{

}
*/


EXPORT void CALL CloseDLL(void)
{

}


EXPORT void CALL DllAbout(HWND hParent)
{

}


EXPORT void CALL DllConfig(HWND hParent)
{

}


EXPORT void CALL DllTest(HWND hParent)
{

}


EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = 0x0101;
    PluginInfo->Type = PLUGIN_TYPE_AUDIO;
    strcpy_s(
        PluginInfo->Name, "Dummy Audio");
    PluginInfo->NormalMemory = TRUE;
    PluginInfo->MemoryBswaped = TRUE;
    return;
}


EXPORT BOOL CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
    return true;
}


EXPORT void CALL ProcessAList(void)
{

}


EXPORT void CALL RomClosed(void)
{

}


