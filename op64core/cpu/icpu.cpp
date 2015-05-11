#include "icpu.h"

#include <core/bus.h>
#include "cp0.h"
#include "cp1.h"
#include "interrupthandler.h"


ICPU::ICPU(void) :
_PC(0),
_cur_instr({ 0 }),
_cp0(new CP0()),
_cp1(new CP1(&_s_reg, &_d_reg, &_fgr)),
_delay_slot(false)
{
    using namespace Bus;
    PC = &_PC;

    // Needs to be initialized by concrete cpu
    interrupt = new InterruptHandler();
}

ICPU::~ICPU(void)
{
    if (nullptr != _cp0)
    {
        delete _cp0; _cp0 = nullptr;
    }

    if (nullptr != _cp1)
    {
        delete _cp1; _cp1 = nullptr;
    }

    using namespace Bus;
    PC = nullptr;

    if (nullptr != interrupt)
    {
        delete interrupt; interrupt = nullptr;
    }
}

