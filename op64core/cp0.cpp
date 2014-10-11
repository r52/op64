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

void CP0::update_count(void)
{
    _cp0_reg[CP0_COUNT_REG] += ((((uint32_t)*(Bus::PC)) - *(Bus::last_instr_addr)) >> 2) * Bus::rom->getCountPerOp();       // count_per_op???
    *(Bus::last_instr_addr) = (uint32_t)*(Bus::PC);
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
