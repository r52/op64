#pragma once

#include <atomic>
#include <memory>

#include <op64.h>

#include <core/state.h>
#include <cpu/interrupthandler.h>
#include <rom/rom.h>
#include <pif/pif.h>
#include <core/systiming.h>
#include <cheat/cheatengine.h>
#include <rcp/rcp.h>
#include <rom/sram.h>
#include <rom/flashram.h>

class ICPU;
class IMemory;
class PluginContainer;

class Bus final
{
    enum BusState
    {
        BUS_DEAD,
        BUS_DEV_CONNECTED,
        BUS_DEV_INITIALIZED,
    };

public:
    Bus(Rom* r);
    ~Bus();

    Bus(const Bus&) = delete;
    Bus& operator= (const Bus&) = delete;

    bool connectDevices(ICPU* c, IMemory* m, PluginContainer* p);
    bool initializeDevices(void);

    void executeMachine(void);

    void doSoftReset(void);

private:
    void freeEverything(void);

public:
    // Unowned stuff
    ICPU* cpu = nullptr;
    IMemory* mem = nullptr;
    PluginContainer* plugins = nullptr;

    // Owned stuff
    std::unique_ptr<SysTiming> systimer;
    std::unique_ptr<CheatEngine> cheat;
    std::unique_ptr<Rom> rom;
    std::unique_ptr<InterruptHandler> interrupt;
    std::unique_ptr<PIF> pif;
    std::unique_ptr<SRAM> sram;
    std::unique_ptr<FlashRam> flashram;

    // RCP/RAM/State needs to be persistent :(
    static RCP rcp;
    static RDRAMController rdram;
    static ProgramState state;

private:
    BusState _state = BUS_DEAD;
};
