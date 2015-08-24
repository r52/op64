#pragma once

#include <core/inputtypes.h>
#include <cpu/cputypes.h>

struct ProgramState
{
    // cpu state
    ProgramCounter PC = 0;
    uint32_t last_jump_addr;
    uint32_t next_interrupt;
    uint32_t skip_jump;

    // vi state
    uint32_t next_vi;
    uint32_t vi_delay;
    uint32_t vi_field;

    // cp0 regs needs to be global :(
    uint32_t cp0_reg[CP0_REGS_COUNT];

    // interrupt state
    bool interrupt_unsafe_state;

    CONTROL controllers[4];
};
