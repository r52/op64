#include "icpu.h"

ICPU::ICPU(void) :
    _cur_instr({ 0 }),
    _delay_slot(false)
{
}

