#include "mppinterpreter.h"
#include "util.h"
#include "logger.h"
#include "bus.h"

void MPPInterpreter::BLTZ(void)
{
    // DECLARE_JUMP(BLTZ, PCADDR + (iimmediate+1)*4, irs < 0, &reg[0], 0, 0)

    /*
    uint32_t target = (((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2));
    bool condition = ((int64_t)_reg[_cur_instr.rs] < 0);
    if (target == ((uint32_t)_PC) && _check_nop)
    {
        generic_idle(target, condition, &_reg[0], false, false);
        return;
    }

    generic_jump(target, condition, &_reg[0], false, false);
    */

    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s < 0,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::BGEZ(void)
{
    // DECLARE_JUMP(BGEZ, PCADDR + (iimmediate + 1) * 4, irs >= 0, &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s >= 0,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::BLTZL(void)
{
    // DECLARE_JUMP(BLTZL,   PCADDR + (iimmediate+1)*4, irs < 0,    &reg[0],  1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s < 0,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::BGEZL(void)
{
    // DECLARE_JUMP(BGEZL,   PCADDR + (iimmediate+1)*4, irs >= 0,   &reg[0],  1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s >= 0,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::TGEI(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TGEIU(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TLTI(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TLTIU(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TEQI(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TNEI(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::BLTZAL(void)
{
    // DECLARE_JUMP(BLTZAL, PCADDR + (iimmediate + 1) * 4, irs < 0, &reg[31], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s < 0,
        &_reg[31],
        false,
        false
        );
}

void MPPInterpreter::BGEZAL(void)
{
    // DECLARE_JUMP(BGEZAL, PCADDR + (iimmediate + 1) * 4, irs >= 0, &reg[31], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s >= 0,
        &_reg[31],
        false,
        false
        );
}

void MPPInterpreter::BLTZALL(void)
{
    // DECLARE_JUMP(BLTZALL, PCADDR + (iimmediate+1)*4, irs < 0,    &reg[31], 1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s < 0,
        &_reg[31],
        true,
        false
        );
}

void MPPInterpreter::BGEZALL(void)
{
    // DECLARE_JUMP(BGEZALL, PCADDR + (iimmediate+1)*4, irs >= 0,   &reg[31], 1, 0)

    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s >= 0,
        &_reg[31],
        true,
        false
        );
}
