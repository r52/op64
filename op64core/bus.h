#pragma once

#include "op64.h"
#include <atomic>

// forward decls to avoid include creep
class IMemory;
class Rom;
class ICPU;
class ProgramCounter;
union Instruction;
union Register64;
class InterruptHandler;
class PIF;
class Plugins;
class SysTiming;
class CheatEngine;
class RCP;

class RDRAMController;

struct _controller_data;
typedef struct _controller_data CONTROL;
class SRAM;
class FlashRam;

namespace Bus
{
    // unmanaged devices
    extern ICPU* cpu;
    extern IMemory* mem;
    extern Rom* rom;
    extern Plugins* plugins;
    extern SysTiming* systimer;
    extern CheatEngine* cheat;

    // managed devices
    extern RCP* rcp;
    extern InterruptHandler* interrupt;
    extern PIF* pif;
    extern CONTROL* controllers;
    extern SRAM* sram;
    extern FlashRam* flashram;

    // controllers
    extern RDRAMController* rdram;

    // regs
    extern uint32_t* cp0_reg;

    // cpu state
    extern ProgramCounter* PC;
    extern uint32_t last_jump_addr;
    extern uint32_t next_interrupt;
    extern uint32_t skip_jump;
    extern std::atomic<bool> stop;

    // vi state
    extern uint32_t next_vi;
    extern uint32_t vi_delay;
    extern int32_t vi_field;

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
    bool connectPlugins(Plugins* dev);

    bool initializeDevices(void);

    void executeMachine(void);

    bool disconnectDevices(void);

    void doSoftReset(void);
};