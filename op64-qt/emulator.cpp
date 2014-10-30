#include "emulator.h"
#include "rom.h"
#include "bus.h"
#include "plugins.h"
#include "mppinterpreter.h"
#include "mpmemory.h"


Emulator::Emulator(Plugins* plugins) :
_state(DEAD),
_plugins(plugins),
_cpu(new MPPInterpreter),
_mem(new MPMemory)
{
}

Emulator::~Emulator()
{
    Bus::disconnectDevices();

    if (nullptr != _cpu)
    {
        delete _cpu; _cpu = nullptr;
    }

    if (nullptr != _mem)
    {
        delete _mem; _mem = nullptr;
    }

    // Doesn't own it
    _plugins = nullptr;
}

bool Emulator::isRomLoaded(void)
{
    return (nullptr != Bus::rom && getState() > DEAD);
}

bool Emulator::loadRom(const char* filename)
{
    if (getState() > DEAD)
    {
        // something already loaded/running. cant do this
        return false;
    }

    Rom* rom = new Rom();
    if (rom->loadRom(filename))
    {
        Bus::connectRom(rom); rom = nullptr;
        setState(ROM_LOADED);
        return true;
    }
    else
    {
        // something went wrong. kill it
        Bus::disconnectDevices();
        delete rom; rom = nullptr;
    }

    return false;
}

bool Emulator::initializeHardware(Plugins* plugins, ICPU* cpu, IMemory* mem)
{
    if (getState() != ROM_LOADED)
    {
        // not at correct state
        return false;
    }

    setupBus(plugins, cpu, mem);

    if (!Bus::initializeDevices())
    {
        Bus::disconnectDevices();
        setState(DEAD);
        return false;
    }

    setState(HARDWARE_INITIALIZED);
    return true;
}

bool Emulator::execute(void)
{
    if (getState() != HARDWARE_INITIALIZED)
    {
        return false;
    }

    setState(EMU_RUNNING);

    Bus::executeMachine();

    setState(EMU_STOPPED);
    return true;
}

bool Emulator::uninitializeHardware(void)
{
    if (getState() != EMU_STOPPED)
    {
        return false;
    }

    setState(DEAD);

    if (Bus::disconnectDevices())
    {
        return true;
    }

    return false;
}

void Emulator::stopEmulator(void)
{
    if (getState() != EMU_RUNNING)
    {
        return;
    }

    Bus::stop = true;
}

void Emulator::setState(EmuState newstate)
{
    _state = newstate;

    emit stateChanged(_state);
}

void Emulator::setupBus(Plugins* plugins, ICPU* cpu, IMemory* mem)
{
    using namespace Bus;

    // create devices
    Bus::connectMemory(mem);
    Bus::connectCPU(cpu);
    Bus::connectPlugins(plugins);
}

void Emulator::setLimitFPS(bool limit)
{
    Bus::limitVI = limit;
}

void Emulator::gameHardReset(void)
{
    Bus::doHardReset = true;
}

void Emulator::gameSoftReset(void)
{
    Bus::doSoftReset();
}

void Emulator::runEmulator(void)
{
    if (initializeHardware(_plugins, _cpu, _mem))
    {
        execute();

        uninitializeHardware();
    }

    emit emulatorFinished();
}
