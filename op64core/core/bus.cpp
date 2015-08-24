#include <oplog.h>

#include "bus.h"

#include <mem/imemory.h>
#include <cpu/icpu.h>
#include <plugin/plugincontainer.h>


RCP Bus::rcp;
RDRAMController Bus::rdram;
ProgramState Bus::state;


Bus::Bus(Rom* r) :
    rom(r)
{
}


Bus::~Bus()
{
    freeEverything();
}

bool Bus::connectDevices(ICPU* c, IMemory* m, PluginContainer* p)
{
    freeEverything();

    cpu = c;
    mem = m;
    plugins = p;

    if (cpu && mem && plugins && rom)
    {
        _state = BUS_DEV_CONNECTED;
        return true;
    }

    LOG_ERROR(Bus) << "One or more connected devices are invalid";

    return false;
}

bool Bus::initializeDevices(void)
{
    LOG_INFO(Bus) << "Initializing devices";

    if (_state < BUS_DEV_CONNECTED)
    {
        LOG_ERROR(Bus) << "One or more connected devices not connected";
        return false;
    }

    if (mem->initialize(this) &&
        cpu->initialize(this) &&
        plugins->initialize(this))
    {

        systimer.reset(new SysTiming(rom->getViLimit()));
        cheat.reset(new CheatEngine);

        LOG_INFO(Bus) << "Devices successfully initialized";
        _state = BUS_DEV_INITIALIZED;

        return true;
    }

    LOG_ERROR(Bus) << "One or more plugins failed to initialize";

    return false;
}

void Bus::executeMachine(void)
{
    if (_state < BUS_DEV_INITIALIZED)
    {
        LOG_ERROR(Bus) << "Execute failed. Devices not initialized";
        return;
    }

    LOG_INFO(Bus) << "Executing emulator...";

    plugins->RomOpened();

    systimer->startTimers();
    cheat->addRomHacks(rom->getRomHacks());

    cpu->execute();

    LOG_INFO(Bus) << "Stopping emulator...";

    plugins->RomClosed();

    LOG_INFO(Bus) << "Uninitializing devices";

    mem->uninitialize(this);
    cpu->uninitialize(this);

    plugins->StopEmulation();
    plugins->uninitialize(this);

    freeEverything();

    _state = BUS_DEAD;
}

void Bus::doSoftReset(void)
{
    if (_state < BUS_DEV_INITIALIZED)
    {
        LOG_ERROR(Bus) << "Reset error. Devices not initialized";
        return;
    }

    LOG_INFO(Bus) << "Soft resetting emulator...";

    interrupt->softReset();
}

void Bus::freeEverything(void)
{
    cpu = nullptr;
    mem = nullptr;
    plugins = nullptr;

    systimer.reset();
    cheat.reset();
    interrupt.reset();
    pif.reset();
    sram.reset();
    flashram.reset();
}
