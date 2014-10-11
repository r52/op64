#include <cstdint>
#include <string.h>

#include "Audio #1.1.h"


EXPORT void CALL AiDacrateChanged(int SystemType)
{

}


EXPORT void CALL AiLenChanged(void)
{

}


EXPORT uint32_t CALL AiReadLength(void)
{
    return 0;
}

/*
EXPORT void CALL AiUpdate(int Wait)
{

}
*/


EXPORT void CALL CloseDLL(void)
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
    PluginInfo->Type = PLUGIN_TYPE_AUDIO;
    strcpy(PluginInfo->Name, "Dummy Audio");
    PluginInfo->NormalMemory = 1;
    PluginInfo->MemoryBswaped = 1;
    return;
}


EXPORT int CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
    return true;
}


EXPORT void CALL ProcessAList(void)
{

}


EXPORT void CALL RomClosed(void)
{

}


