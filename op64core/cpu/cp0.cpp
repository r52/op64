#include "cp0.h"
#include "icpu.h"

#include <core/bus.h>
#include <rom/rom.h>

CP0::CP0()
{
    // forward internals
    Bus::cp0_reg = _cp0_reg;
}

CP0::~CP0()
{
    Bus::cp0_reg = nullptr;
}

void CP0::updateCount(uint32_t PC)
{
    _cp0_reg[CP0_COUNT_REG] += ((PC - Bus::last_jump_addr) >> 2) * Bus::rom->getCountPerOp();
    _cp0_reg[CP0_RANDOM_REG] = (_cp0_reg[CP0_COUNT_REG] / 2 % (32 - _cp0_reg[CP0_WIRED_REG]))
        + _cp0_reg[CP0_WIRED_REG];
    Bus::last_jump_addr = PC;
}

bool CP0::COP1Unusable(void)
{
    if (!(_cp0_reg[CP0_STATUS_REG] & 0x20000000))
    {
        _cp0_reg[CP0_CAUSE_REG] = (11 << 2) | 0x10000000;
        Bus::cpu->generalException();
        return true;
    }
    return false;
}