#include "emulatorthread.h"
#include "emulator.h"
#include "mppinterpreter.h"
#include "mpmemory.h"


void EmulatorThread::runEmulator(void)
{
    if (EMU.initializeHardware(_plugins, _cpu, _mem))
    {
        EMU.execute();

        EMU.uninitializeHardware();
    }

    emit emulatorFinished();
}

EmulatorThread::EmulatorThread(Plugins* plugins)
    :
    _plugins(plugins),
    _cpu(new MPPInterpreter),
    _mem(new MPMemory)
{
}

EmulatorThread::~EmulatorThread()
{
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

