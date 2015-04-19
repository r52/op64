#include <functional>
#include <boost\filesystem.hpp>

#include <logger.h>

#include "plugins.h"

#include "gfxplugin.h"
#include "audioplugin.h"
#include "rspplugin.h"
#include "inputplugin.h"

#include <core/bus.h>


Plugins::Plugins() :
_gfx(nullptr),
_audio(nullptr),
_rsp(nullptr),
_input(nullptr),
_renderWindow(nullptr),
_statusBar(nullptr)
{
    loadPlugins();

    _plugChangeCallback = ConfigStore::getInstance().registerCallback(std::bind(&Plugins::pluginsChanged, this));

    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_GFX_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_RSP_PLUGIN, _plugChangeCallback);
    ConfigStore::getInstance().addChangeCallback(CFG_SECTION_CORE, CFG_INPUT_PLUGIN, _plugChangeCallback);
}

Plugins::~Plugins()
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

void Plugins::setRenderWindow(void* windowHandle)
{
    _renderWindow = windowHandle;
}


void Plugins::setStatusBar(void* statusBar)
{
    _statusBar = statusBar;
}

void Plugins::pluginsChanged(void)
{
    if (!Bus::stop)
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

void Plugins::loadPlugins(void)
{
    LOG_INFO("Plugins: Loading plugins");

    if (nullptr == _gfx)
    {
        _gfxPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_GFX_PLUGIN);
        LOG_VERBOSE("Graphics Plugin: %s", _gfxPath.c_str());
        _gfx = new GfxPlugin(_gfxPath.c_str());
        ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_GFX_NAME, _gfx->PluginName().c_str());
    }

    if (nullptr == _audio)
    {
        _audioPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN);
        LOG_VERBOSE("Audio Plugin: %s", _audioPath.c_str());
        _audio = new AudioPlugin(_audioPath.c_str());
        ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_AUDIO_NAME, _audio->PluginName().c_str());
    }

    if (nullptr == _rsp)
    {
        _rspPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_RSP_PLUGIN);
        LOG_VERBOSE("RSP Plugin: %s", _rspPath.c_str());
        _rsp = new RSPPlugin(_rspPath.c_str());
        ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_RSP_NAME, _rsp->PluginName().c_str());
    }

    if (nullptr == _input)
    {
        _inputPath = ConfigStore::getInstance().getString(CFG_SECTION_CORE, CFG_INPUT_PLUGIN);
        LOG_VERBOSE("Input Plugin: %s", _inputPath.c_str());
        _input = new InputPlugin(_inputPath.c_str());
        ConfigStore::getInstance().set(CFG_SECTION_CORE, CFG_INPUT_NAME, _input->PluginName().c_str());
    }
}

bool Plugins::ValidPluginVersion(PLUGIN_INFO & PluginInfo)
{
    switch (PluginInfo.Type)
    {
    case PLUGIN_TYPE_RSP:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0001) { return true; }
        if (PluginInfo.Version == 0x0100) { return true; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        break;
    case PLUGIN_TYPE_GFX:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0102) { return true; }
        if (PluginInfo.Version == 0x0103) { return true; }
        if (PluginInfo.Version == 0x0104) { return true; }
        break;
    case PLUGIN_TYPE_AUDIO:
        if (!PluginInfo.MemoryBswaped)	  { return false; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (PluginInfo.Version == 0x0100) { return true; }
        if (PluginInfo.Version == 0x0101) { return true; }
        if (PluginInfo.Version == 0x0102) { return true; }
        break;
    }

    return false;
}

void Plugins::Reset(void)
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

void Plugins::DestroyGfxPlugin(void)
{
    if (nullptr == _gfx)
    {
        return;
    }

    LOG_INFO("Plugins: Destroying Graphics plugin");

    delete _gfx; _gfx = nullptr;
    DestroyRspPlugin();
}

void Plugins::DestroyAudioPlugin(void)
{
    if (nullptr == _audio)
    {
        return;
    }

    LOG_INFO("Plugins: Destroying Audio plugin");

    _audio->close();
    delete _audio;  _audio = nullptr;
    DestroyRspPlugin();
}

void Plugins::DestroyRspPlugin(void)
{
    if (nullptr == _rsp)
    {
        return;
    }

    LOG_INFO("Plugins: Destroying RSP plugin");

    _rsp->close();
    delete _rsp; _rsp = nullptr;
}

void Plugins::DestroyInputPlugin(void)
{
    if (nullptr == _input)
    {
        return;
    }

    LOG_INFO("Plugins: Destroying Input plugin");

    _input->close();
    delete _input; _input = nullptr;
}

void Plugins::GameReset(void)
{
    if (_gfx)   {
        _gfx->GameReset();
    }
    if (_audio)   {
        _audio->GameReset();
    }
    if (_rsp)   {
        _rsp->GameReset();
    }
    if (_input) {
        _input->GameReset();
    }
}

void Plugins::RomOpened(void)
{
    LOG_INFO("Plugins: ROM opened");
    _gfx->onRomOpen();
    _rsp->onRomOpen();
    _audio->onRomOpen();
    _input->onRomOpen();
}

void Plugins::RomClosed(void)
{
    LOG_INFO("Plugins: ROM closed");
    _gfx->onRomClose();
    _rsp->onRomClose();
    _audio->onRomClose();
    _input->onRomClose();
}

bool Plugins::initialize(void)
{
    LOG_INFO("Plugins: initializing...");

    if (_gfx == nullptr)
    {
        LOG_ERROR("Plugins: No graphics plugin loaded");
        return false;
    }

    if (_audio == nullptr)
    {
        LOG_ERROR("Plugins: No audio plugin loaded");
        return false;
    }
    if (_rsp == nullptr)
    {
        LOG_ERROR("Plugins: No rsp plugin loaded");
        return false;
    }

    if (_input == nullptr)
    {
        LOG_ERROR("Plugins: No input plugin loaded");
        return false;
    }

    if (!_gfx->initialize(_renderWindow, _statusBar))
    {
        LOG_ERROR("Plugins: Graphics plugin failed to initialize");
        return false;
    }

    if (!_audio->initialize(_renderWindow, _statusBar))
    {
        LOG_ERROR("Plugins: Audio plugin failed to initialize");
        return false;
    }

    if (!_input->initialize(_renderWindow, _statusBar))
    {
        LOG_ERROR("Plugins: Input plugin failed to initialize");
        return false;
    }

    if (!_rsp->initialize(this, _renderWindow, _statusBar))
    {
        LOG_ERROR("Plugins: RSP plugin failed to initialize");
        return false;
    }

    LOG_INFO("Plugins: successfully initialized");

    return true;
}

void Plugins::ConfigPlugin(void* parentWindow, PLUGIN_TYPE Type)
{
    switch (Type) {
    case PLUGIN_TYPE_RSP:
        if (nullptr == _rsp || nullptr == _rsp->Config) { break; }
        if (!_rsp->isInitialized()) {
            if (!_rsp->initialize(this, nullptr, nullptr)) {
                break;
            }
        }
        _rsp->Config(parentWindow);
        break;
    case PLUGIN_TYPE_GFX:
        if (_gfx == nullptr || _gfx->Config == nullptr) { break; }
        if (!_gfx->isInitialized()) {
            if (!_gfx->initialize(nullptr, nullptr)) {
                break;
            }
        }
        _gfx->Config(parentWindow);
        break;
    case PLUGIN_TYPE_AUDIO:
        if (_audio == nullptr || _audio->Config == nullptr) { break; }
        if (!_audio->isInitialized()) {
            if (!_audio->initialize(nullptr, nullptr)) {
                break;
            }
        }
        _audio->Config(parentWindow);
        break;
    case PLUGIN_TYPE_CONTROLLER:
        if (_input == nullptr || _input->Config == nullptr) { break; }
        if (!_input->isInitialized()) {
            if (!_input->initialize(nullptr, nullptr)) {
                break;
            }
        }
        _input->Config(parentWindow);
        break;

    }
}

void Plugins::StopEmulation(void)
{
    DestroyGfxPlugin();
    DestroyAudioPlugin();
    DestroyRspPlugin();
    DestroyInputPlugin();

    loadPlugins();
}

void DummyFunction(void)
{
}
