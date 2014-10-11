#pragma once

#include <cstdint>
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

    // managed devices
    extern InterruptHandler* interrupt;
    extern PIF* pif;
    extern CONTROL* controllers;
    extern SRAM* sram;
    extern FlashRam* flashram;


    // regs
    extern Register64* reg;
    extern Register64* hi;
    extern Register64* lo;
    extern bool* llbit;
    extern uint32_t* cp0_reg;
    extern uint32_t* FCR0;
    extern uint32_t* FCR31;
    extern float** s_reg;
    extern double** d_reg;
    extern uint64_t* fgr;

    // cpu state
    extern ProgramCounter* PC;
    extern uint32_t* last_instr_addr;
    extern uint32_t* next_interrupt;
    extern Instruction* cur_instr;
    extern uint32_t* skip_jump;
    extern std::atomic<bool> stop;
    extern bool* delay_slot;

    // mem
    extern uint8_t* rom_image;
    extern uint32_t* sp_dmem32;
    extern uint32_t* sp_imem32;
    extern uint8_t* sp_dmem8;
    extern uint8_t* sp_imem8;
    extern uint32_t* rdram;
    extern uint8_t* rdram8;
    extern uint32_t* pif_ram32;
    extern uint8_t* pif_ram8;


    // in-memory regs
    extern uint32_t* vi_reg;
    extern uint32_t* mi_reg;
    extern uint32_t* rdram_reg;
    extern uint32_t* sp_reg;
    extern uint32_t* ai_reg;
    extern uint32_t* pi_reg;
    extern uint32_t* ri_reg;
    extern uint32_t* si_reg;
    extern uint32_t* dp_reg;
    extern uint32_t* dps_reg;

    // vi state
    extern uint32_t* next_vi;
    extern int32_t* vi_field;

    // interrupt state
    extern bool* SPECIAL_done;
    extern bool* perform_hard_reset;
    extern bool* interrupt_unsafe_state;

    bool connectRom(Rom* dev);
    bool connectMemory(IMemory* dev);
    bool connectCPU(ICPU* dev);
    bool connectPlugins(Plugins* dev);

    bool initializeDevices(void);

    void executeMachine(void);

    bool disconnectDevices(void);
};