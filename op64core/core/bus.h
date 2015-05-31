#pragma once

#include <atomic>

#include <op64.h>

#include "inputtypes.h"

// forward decls to avoid include creep
class IMemory;
class Rom;
class ICPU;
class ProgramCounter;
union Instruction;
union Register64;
class InterruptHandler;
class PIF;
class PluginContainer;
class SysTiming;
class CheatEngine;
class RCP;

class RDRAMController;

class SRAM;
class FlashRam;

namespace Bus
{
    // unmanaged devices
    extern ICPU* cpu;
    extern IMemory* mem;
    extern PluginContainer* plugins;

    // managed devices
    extern SysTiming* systimer;
    extern CheatEngine* cheat;
    extern Rom* rom;
    extern InterruptHandler* interrupt;
    extern PIF* pif;
    extern SRAM* sram;
    extern FlashRam* flashram;

    // controllers
    extern RCP* rcp;
    extern RDRAMController* rdram;

    // regs
    extern uint32_t* cp0_reg;

    // controller data
    extern CONTROL controllers[4];

    // cpu state
    extern ProgramCounter* PC;
    extern uint32_t last_jump_addr;
    extern uint32_t next_interrupt;
    extern uint32_t skip_jump;
    extern std::atomic<bool> stop;

    // vi state
    extern uint32_t next_vi;
    extern uint32_t vi_delay;
    extern uint32_t vi_field;

    // interrupt state
    extern bool interrupt_unsafe_state;

    // core control
    extern std::atomic<bool> doHardReset;
    extern std::atomic<bool> limitVI;

    OPStatus BusStartup(void);
    OPStatus BusShutdown(void);

    bool connectRom(Rom* dev);
    bool connectMemory(IMemory* dev);
    bool connectCPU(ICPU* dev);
    bool connectPlugins(PluginContainer* dev);

    bool initializeDevices(void);

    void executeMachine(void);

    bool disconnectDevices(void);

    void doSoftReset(void);
};