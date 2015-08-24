#include "cp0.h"
#include "icpu.h"

#include <core/bus.h>
#include <rom/rom.h>


void CP0::updateCount(uint32_t PC, uint8_t countPerOp)
{
    Bus::state.cp0_reg[CP0_COUNT_REG] += ((PC - Bus::state.last_jump_addr) >> 2) * countPerOp;
    Bus::state.cp0_reg[CP0_RANDOM_REG] = (Bus::state.cp0_reg[CP0_COUNT_REG] / 2 % (32 - Bus::state.cp0_reg[CP0_WIRED_REG]))
        + Bus::state.cp0_reg[CP0_WIRED_REG];
    Bus::state.last_jump_addr = PC;
}

bool CP0::COP1Unusable(ICPU& cpu)
{
    if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & 0x20000000))
    {
        Bus::state.cp0_reg[CP0_CAUSE_REG] = (11 << 2) | 0x10000000;
        cpu.generalException();
        return true;
    }
    return false;
}
