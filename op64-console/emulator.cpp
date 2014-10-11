#include "emulator.h"
#include "mppinterpreter.h"
#include "mpmemory.h"
#include "rom.h"
#include "bus.h"
#include "wrdp.h"
#include "hlersp.h"
#include "llersp.h"
#include "dummyinput.h"
#include "dummyaudio.h"


Emulator::~Emulator()
{
    cleanup();
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

    using namespace Bus;
    if (nullptr != rom)
    {
        // something else already here but we're in correct state so kill it
        delete rom; rom = nullptr;
    }

    rom = new Rom();
    if (rom->loadRom(filename))
    {
        _state = ROM_LOADED;
        return true;
    }
    else
    {
        // something went wrong. kill it
        delete rom; rom = nullptr;
    }

    return false;
}

bool Emulator::initializeHardware(void)
{
    if (getState() != ROM_LOADED)
    {
        // not at correct state
        return false;
    }

    using namespace Bus;
    cleanupDevices();

    // initiate devices
    mem = new MPMemory();
    cpu = new MPPInterpreter();
    rdp = new WRDP();
    rsp = new LLERSP();
    input = new DummyInput();
    audio = new DummyAudio();

    mem->initialize();
    cpu->initialize();
    reinterpret_cast<WRDP*>(rdp)->initialize(_renderWindow);
    rsp->initialize();
    input->initialize(Bus::controllers);
    audio->initialize();

    _state = HARDWARE_INITIALIZED;
    return true;
}

bool Emulator::execute(void)
{
    if (getState() != HARDWARE_INITIALIZED)
    {
        return false;
    }

    _state = EMU_RUNNING;

    // TODO future: thread this
    Bus::cpu->execute();

    _state = EMU_STOPPED;
    return true;
}

bool Emulator::uninitializeHardware(void)
{
    if (getState() != EMU_STOPPED)
    {
        return false;
    }

    cleanup();

    _state = DEAD;

    return true;
}

void Emulator::cleanup(void)
{
    using namespace Bus;
    
    cleanupDevices();

    if (nullptr != rom)
    {
        delete rom; rom = nullptr;
    }

}

void Emulator::cleanupDevices(void)
{
    using namespace Bus;
    if (nullptr != mem)
    {
        delete mem; mem = nullptr;
    }

    if (nullptr != cpu)
    {
        delete cpu; cpu = nullptr;
    }

    if (nullptr != rsp)
    {
        delete rsp; rsp = nullptr;
    }

    if (nullptr != rdp)
    {
        delete rdp; rdp = nullptr;
    }

    if (nullptr != input)
    {
        delete input; input = nullptr;
    }

    if (nullptr != audio)
    {
        delete audio; audio = nullptr;
    }
}

void Emulator::stopEmulator(void)
{
    if (getState() != EMU_RUNNING)
    {
        return;
    }

    *(Bus::stop) = true;
}


