#include <QThread>

#include "emulator.h"

#include <rom/rom.h>
#include <plugin/gfxplugin.h>
#include <cpu/interpreter.h>
#include <mem/mpmemory.h>

#include <ui/corecontrol.h>


Emulator::Emulator(WId mainwindow) :
_state(DEAD),
_plugins(new PluginContainer),
_cpu(new Interpreter),
_mem(new MPMemory),
_mainwindow(mainwindow)
{
    qRegisterMetaType<EmuState>("EmuState");
}

bool Emulator::isRomLoaded(void)
{
    return (_bus && _bus->rom && getState() > DEAD);
}

bool Emulator::loadRom(const char* filename, QString& romname)
{
    if (getState() > DEAD)
    {
        // something already loaded/running. cant do this
        return false;
    }

    Rom* rom;
    if (Rom::loadRom(filename, rom))
    {
        romname = QString::fromLocal8Bit((char*)rom->getHeader()->Name, 20).trimmed();

        _bus.reset(new Bus(rom));
        setState(ROM_LOADED);
        return true;
    }

    return false;
}

bool Emulator::initializeHardware(PluginContainer* plugins, ICPU* cpu, IMemory* mem)
{
    if (!_bus || getState() != ROM_LOADED)
    {
        // not at correct state
        return false;
    }

    if (_bus && _bus->connectDevices(cpu, mem, plugins) && _bus->initializeDevices())
    {
        setState(HARDWARE_INITIALIZED);
        return true;
    }

    setState(DEAD);
    return false;
}

bool Emulator::execute(void)
{
    if (!_bus || getState() != HARDWARE_INITIALIZED)
    {
        return false;
    }

    setState(EMU_RUNNING);

    _bus->executeMachine();

    setState(EMU_STOPPED);
    return true;
}

void Emulator::stopEmulator(void)
{
    if (getState() != EMU_RUNNING)
    {
        return;
    }

    CoreControl::stop = true;
}

void Emulator::setLimitFPS(bool limit)
{
    LOG_INFO(EmulatorThread) << "Speed limit " << limit ? "on" : "off";
    CoreControl::limitVI = limit;
}

void Emulator::gameHardReset(void)
{
    LOG_INFO(EmulatorThread) << "Hard resetting emulator...";
    CoreControl::doHardReset = true;
}

void Emulator::gameSoftReset(void)
{
    if (_bus)
    {
        _bus->doSoftReset();
    }
}

void Emulator::runEmulator(void)
{
    // Log some threading info
    LOG_DEBUG(EmulatorThread) << "Emulating in thread ID " << (uintptr_t) QThread::currentThreadId();

    _plugins->setRenderWindow((void*)_renderwindow);

    if (initializeHardware(_plugins.get(), _cpu.get(), _mem.get()))
    {
        execute();
    }

    setState(DEAD);

    emit emulatorFinished();
}

void Emulator::showAudioConfig(void)
{
    _plugins->ConfigPlugin((void*)_mainwindow, PLUGIN_TYPE_AUDIO);
}

void Emulator::showInputConfig(void)
{
    _plugins->ConfigPlugin((void*)_mainwindow, PLUGIN_TYPE_CONTROLLER);
}

void Emulator::showGraphicsConfig(void)
{
    _plugins->ConfigPlugin((void*)_mainwindow, PLUGIN_TYPE_GFX);
}

void Emulator::showRSPConfig(void)
{
    _plugins->ConfigPlugin((void*)_mainwindow, PLUGIN_TYPE_RSP);
}

void Emulator::toggleFullScreen(void)
{
    _plugins->gfx()->ChangeWindow();
}

void Emulator::setState(EmuState newstate)
{
    _state = newstate;

    emit stateChanged(_state);
}
