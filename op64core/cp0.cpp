#include "cp0.h"
#include "icpu.h"
#include "rom.h"

CP0::CP0()
{
    // forward internals
    Bus::cp0_reg = _cp0_reg;
}

CP0::~CP0()
{
    Bus::cp0_reg = nullptr;
}

void CP0::update_count(uint32_t PC)
{
    _cp0_reg[CP0_COUNT_REG] += ((PC - Bus::last_jump_addr) >> 2) * Bus::rom->getCountPerOp();
    Bus::last_jump_addr = PC;
}

bool CP0::cop1_unusable(void)
{
    if (!(_cp0_reg[CP0_STATUS_REG] & 0x20000000))
    {
        _cp0_reg[CP0_CAUSE_REG] = (11 << 2) | 0x10000000;
        Bus::cpu->general_exception();
        return true;
    }
    return false;
}
