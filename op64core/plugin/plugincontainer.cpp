#include <functional>
#include <boost/filesystem.hpp>

#include <oplog.h>
#include <globalstrings.h>

#include "plugincontainer.h"

#include "gfxplugin.h"
#include "audioplugin.h"
#include "rspplugin.h"
#include "inputplugin.h"

#include <core/bus.h>

#include <ui/corecontrol.h>

using namespace GlobalStrings;


PluginContainer::PluginContainer() :
_gfx(nullptr),
_audio(nullptr),
_rsp(nullptr),
_input(nullptr),
_renderWindow(nullptr),
_statusBar(nullptr)
{
    loadPlugins();

    _plugChangeCallback = ConfigStore::getInstance().registerCallback(std::bind(&PluginContainer::pluginsChanged, this));

    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_GFX_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_RSP_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_INPUT_PLUGIN, _plugChangeCallback);
}

PluginContainer::~PluginContainer()
{
    ConfigStore::getInstance().removeChangeCallback(CFG_SECTION_CORE, CFG_GFX_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().removeChangeCallback(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().removeChangeCallback(CFG_SECTION_CORE, CFG_RSP_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().removeChangeCallback(CFG_SECTION_CORE, CFG_INPUT_PLUGIN, _plugChangeCallback);

    DestroyGfxPlugin();
    DestroyAudioPlugin();
    DestroyRspPlugin();
    DestroyInputPlugin();
}

void PluginContainer::setRenderWindow(void* windowHandle)
{
    _renderWindow = windowHandle;
}


void PluginContainer::setStatusBar(void* statusBar)
{
    _statusBar = statusBar;
}

void PluginContainer::pluginsChanged(void)
{
    if (!CoreControl::stop)
    {
        return;
    }

    bool gfxChange = (_gfxPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_GFX_PLUGIN));
    bool audioChange = (_audioPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN));
    bool rspChange = (_rspPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_RSP_PLUGIN));
    bool inputChange = (_inputPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_INPUT_PLUGIN));

    if (gfxChange || audioChange || rspChange || inputChange)
    {
        Reset();
    }
}

void PluginContainer::loadPlugins(void)
{
    LOG_INFO(PluginContainer) << "Loading plugins";

    if (nullptr == _gfx)
    {
        _gfxPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_GFX_PLUGIN);
        if (OP_OK == GfxPlugin::loadPlugin(_gfxPath.c_str(), _gfx))
        {
            LOG_TRACE(PluginContainer) << "Graphics Plugin: " << _gfxPath;
            ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_GFX_NAME, _gfx->pluginName().c_str());
        }
    }

    if (nullptr == _audio)
    {
        _audioPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN);
        if (OP_OK == AudioPlugin::loadPlugin(_audioPath.c_str(), _audio))
        {
            LOG_TRACE(PluginContainer) << "Audio Plugin: " << _audioPath;
            ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_AUDIO_NAME, _audio->pluginName().c_str());
        }
    }

    if (nullptr == _rsp)
    {
        _rspPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_RSP_PLUGIN);
        if (OP_OK == RSPPlugin::loadPlugin(_rspPath.c_str(), _rsp))
        {
            LOG_TRACE(PluginContainer) << "RSP Plugin: " << _rspPath;
            ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_RSP_NAME, _rsp->pluginName().c_str());
        }
    }

    if (nullptr == _input)
    {
        _inputPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_INPUT_PLUGIN);
        if (OP_OK == InputPlugin::loadPlugin(_inputPath.c_str(), _input))
        {
            LOG_TRACE(PluginContainer) << "Input Plugin: " << _inputPath;
            ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_INPUT_NAME, _input->pluginName().c_str());
        }
    }
}

void PluginContainer::Reset(void)
{
    bool gfxChange = (_gfxPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_GFX_PLUGIN));
    bool audioChange = (_audioPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN));
    bool rspChange = (_rspPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_RSP_PLUGIN));
    bool inputChange = (_inputPath != ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_INPUT_PLUGIN));

    if (gfxChange)
    {
        DestroyGfxPlugin();
    }

    if (audioChange)
    {
        DestroyAudioPlugin();
    }

    if (rspChange)
    {
        DestroyRspPlugin();
    }

    if (inputChange)
    {
        DestroyInputPlugin();
    }

    loadPlugins();
}

void PluginContainer::DestroyGfxPlugin(void)
{
    if (nullptr == _gfx)
    {
        return;
    }

    LOG_INFO(PluginContainer) << "Destroying Graphics plugin";

    delete _gfx; _gfx = nullptr;
    DestroyRspPlugin();
}

void PluginContainer::DestroyAudioPlugin(void)
{
    if (nullptr == _audio)
    {
        return;
    }

    LOG_INFO(PluginContainer) << "Destroying Audio plugin";

    _audio->closePlugin();
    delete _audio;  _audio = nullptr;
    DestroyRspPlugin();
}

void PluginContainer::DestroyRspPlugin(void)
{
    if (nullptr == _rsp)
    {
        return;
    }

    LOG_INFO(PluginContainer) << "Destroying RSP plugin";

    _rsp->closePlugin();
    delete _rsp; _rsp = nullptr;
}

void PluginContainer::DestroyInputPlugin(void)
{
    if (nullptr == _input)
    {
        return;
    }

    LOG_INFO(PluginContainer) << "Destroying Input plugin";

    _input->closePlugin();
    delete _input; _input = nullptr;
}

void PluginContainer::GameReset(void)
{
    if (_gfx)   {
        _gfx->gameReset();
    }
    if (_audio)   {
        _audio->gameReset();
    }
    if (_rsp)   {
        _rsp->gameReset();
    }
    if (_input) {
        _input->gameReset();
    }
}

void PluginContainer::RomOpened(void)
{
    LOG_INFO(PluginContainer) << "ROM opened";
    _gfx->onRomOpen();
    _rsp->onRomOpen();
    _audio->onRomOpen();
    _input->onRomOpen();
}

void PluginContainer::RomClosed(void)
{
    LOG_INFO(PluginContainer) << "ROM closed";
    _gfx->onRomClose();
    _rsp->onRomClose();
    _audio->onRomClose();
    _input->onRomClose();
}

void PluginContainer::uninitialize(Bus* bus)
{

}

bool PluginContainer::initialize(Bus* bus)
{
    LOG_INFO(PluginContainer) << "Initializing...";

    if (_gfx == nullptr)
    {
        LOG_ERROR(PluginContainer) << "No graphics plugin loaded";
        return false;
    }

    if (_audio == nullptr)
    {
        LOG_ERROR(PluginContainer) << "No audio plugin loaded";
        return false;
    }
    if (_rsp == nullptr)
    {
        LOG_ERROR(PluginContainer) << "No rsp plugin loaded";
        return false;
    }

    if (_input == nullptr)
    {
        LOG_ERROR(PluginContainer) << "No input plugin loaded";
        return false;
    }

    if (!_gfx->initialize(bus, this, _renderWindow, _statusBar))
    {
        LOG_ERROR(PluginContainer) << "Graphics plugin failed to initialize";
        return false;
    }

    if (!_audio->initialize(bus, this, _renderWindow, _statusBar))
    {
        LOG_ERROR(PluginContainer) << "Audio plugin failed to initialize";
        return false;
    }

    if (!_input->initialize(bus, this, _renderWindow, _statusBar))
    {
        LOG_ERROR(PluginContainer) << "Input plugin failed to initialize";
        return false;
    }

    if (!_rsp->initialize(bus, this, _renderWindow, _statusBar))
    {
        LOG_ERROR(PluginContainer) << "RSP plugin failed to initialize";
        return false;
    }

    LOG_INFO(PluginContainer) << "Successfully initialized";

    return true;
}

void PluginContainer::ConfigPlugin(void* parentWindow, PLUGIN_TYPE Type)
{
    switch (Type) {
    case PLUGIN_TYPE_RSP:
        if (nullptr == _rsp || nullptr == _rsp->Config) { break; }
        if (!_rsp->isInitialized()) {
            if (!_rsp->initialize(nullptr, this, nullptr, nullptr)) {
                break;
            }
        }
        _rsp->Config(parentWindow);
        break;
    case PLUGIN_TYPE_GFX:
        if (_gfx == nullptr || _gfx->Config == nullptr) { break; }
        if (!_gfx->isInitialized()) {
            if (!_gfx->initialize(nullptr, this, nullptr, nullptr)) {
                break;
            }
        }
        _gfx->Config(parentWindow);
        break;
    case PLUGIN_TYPE_AUDIO:
        if (_audio == nullptr || _audio->Config == nullptr) { break; }
        if (!_audio->isInitialized()) {
            if (!_audio->initialize(nullptr, this, nullptr, nullptr)) {
                break;
            }
        }
        _audio->Config(parentWindow);
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (_input == nullptr || _input->Config == nullptr) { break; }
        if (!_input->isInitialized()) {
            if (!_input->initialize(nullptr, this, nullptr, nullptr)) {
                break;
            }
        }
        _input->Config(parentWindow);
        break;

    }
}

void PluginContainer::StopEmulation(void)
{
    DestroyGfxPlugin();
    DestroyAudioPlugin();
    DestroyRspPlugin();
    DestroyInputPlugin();

    loadPlugins();
}
