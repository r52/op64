#include <oputil.h>
#include <oplog.h>

#include "interpreter.h"

#include <core/bus.h>


void Interpreter::BLTZ(void)
{
    // DECLARE_JUMP(BLTZ, PCADDR + (iimmediate+1)*4, irs < 0, &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s < 0,
        &_reg[0],
        false,
        false
        );
}

void Interpreter::BGEZ(void)
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

void Interpreter::BLTZL(void)
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

void Interpreter::BGEZL(void)
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

void Interpreter::TGEI(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TGEIU(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TLTI(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TLTIU(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TEQI(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TNEI(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::BLTZAL(void)
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

void Interpreter::BGEZAL(void)
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

void Interpreter::BLTZALL(void)
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

void Interpreter::BGEZALL(void)
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
