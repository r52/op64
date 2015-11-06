#pragma once

#include <op64.h>

#define MaxPluginSetting	65535

enum SettingID {
    FirstRSPDefaultSet, LastRSPDefaultSet = FirstRSPDefaultSet + MaxPluginSetting,
    FirstRSPSettings, LastRSPSettings = FirstRSPSettings + MaxPluginSetting,
    FirstGfxDefaultSet, LastGfxDefaultSet = FirstGfxDefaultSet + MaxPluginSetting,
    FirstGfxSettings, LastGfxSettings = FirstGfxSettings + MaxPluginSetting,
    FirstAudioDefaultSet, LastAudioDefaultSet = FirstAudioDefaultSet + MaxPluginSetting,
    FirstAudioSettings, LastAudioSettings = FirstAudioSettings + MaxPluginSetting,
    FirstCtrlDefaultSet, LastCtrlDefaultSet = FirstCtrlDefaultSet + MaxPluginSetting,
    FirstCtrlSettings, LastCtrlSettings = FirstCtrlSettings + MaxPluginSetting,
};

enum SettingType {
    SettingType_Unknown = -1,
    SettingType_ConstString = 0,
    SettingType_ConstValue = 1,
    SettingType_CfgFile = 2,
    SettingType_Registry = 3,
    SettingType_RelativePath = 4,
    TemporarySetting = 5,
    SettingType_RomDatabase = 6,
    SettingType_CheatSetting = 7,
    SettingType_GameSetting = 8,
    SettingType_BoolVariable = 9,
    SettingType_NumberVariable = 10,
    SettingType_StringVariable = 11,
    SettingType_SelectedDirectory = 12,
    SettingType_RdbSetting = 13,
};

enum SettingDataType {
    Data_DWORD = 0, Data_String = 1, Data_CPUTYPE = 2, Data_SelfMod = 3, Data_OnOff = 4, Data_YesNo = 5, Data_SaveChip = 6
};

typedef struct {
    uint32_t  dwSize;
    int    DefaultStartRange;
    int    SettingStartRange;
    int    MaximumSettings;
    int    NoDefault;
    int    DefaultLocation;
    void * handle;
    unsigned int(*GetSetting)      (void * handle, int ID);
    const char * (*GetSettingSz)    (void * handle, int ID, char * Buffer, int BufferLen);
    void(*SetSetting)      (void * handle, int ID, unsigned int Value);
    void(*SetSettingSz)    (void * handle, int ID, const char * Value);
    void(*RegisterSetting) (void * handle, int ID, int DefaultID, SettingDataType Type,
        SettingType Location, const char * Category, const char * DefaultStr, DWORD Value);
    void(*UseUnregisteredSetting) (int ID);
} PLUGIN_SETTINGS;

typedef struct {
    unsigned int(*FindSystemSettingId) (void * handle, const char * Name);
} PLUGIN_SETTINGS2;
