#include "inputplugin.h"

#include <oplog.h>
#include <core/bus.h>
#include <rom/rom.h>

InputPlugin::InputPlugin() :
RumbleCommand(nullptr),
GetKeys(nullptr),
ReadController(nullptr),
ControllerCommand(nullptr)
{
}

InputPlugin::~InputPlugin()
{
    closePlugin();
    unloadPlugin();
}

OPStatus InputPlugin::initialize(Bus* bus, PluginContainer* plugins, void* renderWindow, void* statusBar)
{
    Bus::state.controllers[0].Present = 0;
    Bus::state.controllers[0].RawData = 0;
    Bus::state.controllers[0].Plugin = PLUGIN_NONE;

    Bus::state.controllers[1].Present = 0;
    Bus::state.controllers[1].RawData = 0;
    Bus::state.controllers[1].Plugin = PLUGIN_NONE;

    Bus::state.controllers[2].Present = 0;
    Bus::state.controllers[2].RawData = 0;
    Bus::state.controllers[2].Plugin = PLUGIN_NONE;

    Bus::state.controllers[3].Present = 0;
    Bus::state.controllers[3].RawData = 0;
    Bus::state.controllers[3].Plugin = PLUGIN_NONE;

    //Get DLL information
    void (*GetDllInfo)(PLUGIN_INFO* PluginInfo);

    getPluginFunction(_libHandle, "GetDllInfo", GetDllInfo);
    if (GetDllInfo == nullptr)
    {
        LOG_ERROR(InputPlugin) << "GetDllInfo not found";
        return OP_ERROR;
    }

    PLUGIN_INFO PluginInfo;
    GetDllInfo(&PluginInfo);

    //Test Plugin version
    if (PluginInfo.Version == 0x0100) {
        //Get Function from DLL
        void (*InitiateControllers_1_0)(void* hMainWindow, CONTROL Controls[4]);

        getPluginFunction(_libHandle, "InitiateControllers", InitiateControllers_1_0);
        if (InitiateControllers_1_0 == nullptr)
        {
            LOG_ERROR(InputPlugin) << "InitiateControllers v1.0 not found";
            return OP_ERROR;
        }

        InitiateControllers_1_0(renderWindow, Bus::state.controllers);
        _initialized = true;
    }

    if (PluginInfo.Version >= 0x0101) {
        typedef struct {
            void* hMainWindow;
            void* hinst;

            int MemoryBswaped;
            uint8_t* HEADER;
            CONTROL* Controls;
        } CONTROL_INFO;

        //Get Function from DLL		
        void (*InitiateControllers_1_1)(CONTROL_INFO ControlInfo);

        getPluginFunction(_libHandle, "InitiateControllers", InitiateControllers_1_1);
        if (InitiateControllers_1_1 == nullptr)
        {
            LOG_ERROR(InputPlugin) << "InitiateControllers v1.1 not found";
            return OP_ERROR;
        }

        CONTROL_INFO ControlInfo;
        ControlInfo.Controls = Bus::state.controllers;
        if (bus && bus->rom)
        {
            ControlInfo.HEADER = bus->rom->getImage();
        }
        else
        {
            uint8_t buf[100];
            ControlInfo.HEADER = buf;
        }
        ControlInfo.hinst = opLibGetMainHandle();
        ControlInfo.hMainWindow = renderWindow;
        ControlInfo.MemoryBswaped = 1;
        InitiateControllers_1_1(ControlInfo);
        _initialized = true;
    }

    return OP_OK;
}

OPStatus InputPlugin::loadPlugin(const char * libPath, InputPlugin*& outplug)
{
    if (nullptr != outplug)
    {
        LOG_DEBUG(InputPlugin) << "Existing Plugin object";
        return OP_ERROR;
    }

    InputPlugin* plugin = new InputPlugin;

    if (OP_ERROR == loadLibrary(libPath, plugin->_libHandle, plugin->_pluginInfo))
    {
        delete plugin;
        LOG_ERROR(InputPlugin) << "Error loading library " << libPath;
        return OP_ERROR;
    }

    //Find entries for functions in DLL
    void(*InitFunc)(void);
    
    getPluginFunction(plugin->_libHandle, "RomClosed", plugin->RomClosed);
    getPluginFunction(plugin->_libHandle, "RomOpen", plugin->RomOpen);
    getPluginFunction(plugin->_libHandle, "CloseDLL", plugin->CloseLib);
    getPluginFunction(plugin->_libHandle, "DllConfig", plugin->Config);

    getPluginFunction(plugin->_libHandle, "ControllerCommand", plugin->ControllerCommand);
    getPluginFunction(plugin->_libHandle, "InitiateControllers", InitFunc);
    getPluginFunction(plugin->_libHandle, "ReadController", plugin->ReadController);
    getPluginFunction(plugin->_libHandle, "GetKeys", plugin->GetKeys);
    getPluginFunction(plugin->_libHandle, "RumbleCommand", plugin->RumbleCommand);

    //version 102 functions
    getPluginFunction(plugin->_libHandle, "PluginLoaded", plugin->PluginOpened);

    //Make sure dll had all needed functions
    if (InitFunc == nullptr ||
        plugin->CloseLib == nullptr)
    {
        freeLibrary(plugin->_libHandle);
        delete plugin;
        LOG_ERROR(InputPlugin) << "Invalid plugin: not all required functions are found";
        return OP_ERROR;
    }

    if (plugin->_pluginInfo.Version >= 0x0102)
    {
        if (plugin->PluginOpened == nullptr)
        {
            freeLibrary(plugin->_libHandle);
            delete plugin;
            LOG_ERROR(InputPlugin) << "Invalid plugin: not all required functions are found";
            return OP_ERROR;
        }

        plugin->PluginOpened();
    }

    // Return it
    outplug = plugin; plugin = nullptr;

    return OP_OK;
}

OPStatus InputPlugin::unloadPlugin()
{
    if (OP_ERROR == freeLibrary(_libHandle))
    {
        LOG_FATAL(RSPPlugin) << "Error unloading plugin";
        return OP_ERROR;
    }

    memset(&_pluginInfo, 0, sizeof(_pluginInfo));
    Config = nullptr;
    ControllerCommand = nullptr;
    GetKeys = nullptr;
    ReadController = nullptr;
    CloseLib = nullptr;
    RomOpen = nullptr;
    RomClosed = nullptr;

    return OP_OK;
}

