#include <util.h>
#include <logger.h>

#include "interpreter.h"
#include "cp0.h"

#include <core/bus.h>


#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)
#pragma intrinsic(_mul128)
#endif

void Interpreter::SLL(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>(((int32_t)_reg[_cur_instr.rt].s) << _cur_instr.sa);
    }
    
    ++_PC;
}

void Interpreter::SRL(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rt].u >> _cur_instr.sa);
    }
    
    ++_PC;
}

void Interpreter::SRA(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s >> _cur_instr.sa);
    }

    ++_PC;
}

void Interpreter::SLLV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)(_reg[_cur_instr.rt].s) << ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void Interpreter::SRLV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rt].u >> ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void Interpreter::SRAV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s >> ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void Interpreter::JR(void)
{
    //DECLARE_JUMP(JR,   irs32, 1, &reg[0],    0, 0)

    genericJump((uint32_t)_reg[_cur_instr.rs].u, true, &_reg[0], false, false);
}

void Interpreter::JALR(void)
{
    // DECLARE_JUMP(JALR, irs32, 1, PC->f.r.rd, 0, 0)

    genericJump((uint32_t)_reg[_cur_instr.rs].u, true, &_reg[_cur_instr.rd], false, false);
}

void Interpreter::SYSCALL(void)
{
    _cp0_reg[CP0_CAUSE_REG] = 8 << 2;
    generalException();
}

void Interpreter::BREAK(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SYNC(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::MFHI(void)
{
    _reg[_cur_instr.rd].s = _hi.s;

    ++_PC;
}

void Interpreter::MTHI(void)
{
    _hi.s = _reg[_cur_instr.rs].s;
    ++_PC;
}

void Interpreter::MFLO(void)
{
    _reg[_cur_instr.rd].s = _lo.s;
    
    ++_PC;
}

void Interpreter::MTLO(void)
{
    _lo.s = _reg[_cur_instr.rs].s;
    ++_PC;
}

void Interpreter::DSLLV(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rt].s << ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void Interpreter::DSRLV(void)
{
    _reg[_cur_instr.rd].u = _reg[_cur_instr.rt].u >> ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void Interpreter::DSRAV(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rt].s >> ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void Interpreter::MULT(void)
{
    int64_t result = (int64_t)((int32_t)_reg[_cur_instr.rs].s * (int64_t)((int32_t)_reg[_cur_instr.rt].s));
    _hi.s = signextend<int32_t, int64_t>(result >> 32);
    _lo.s = signextend<int32_t, int64_t>((int32_t)result);
    ++_PC;
}

void Interpreter::MULTU(void)
{
    uint64_t result = (uint64_t)((uint32_t)_reg[_cur_instr.rs].u * (uint64_t)((uint32_t)_reg[_cur_instr.rt].u));
    _hi.s = signextend<int32_t, int64_t>(result >> 32);
    _lo.s = signextend<int32_t, int64_t>((int32_t)result);
    ++_PC;
}

void Interpreter::DIV(void)
{
    if (_reg[_cur_instr.rt].u != 0)
    {
        _lo.s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s / (int32_t)_reg[_cur_instr.rt].s);
        _hi.s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s % (int32_t)_reg[_cur_instr.rt].s);
    }
    else
    {
        LOG_WARNING("DIV: divide by 0");
    }
    ++_PC;
}

void Interpreter::DIVU(void)
{
    if (_reg[_cur_instr.rt].u != 0)
    {
        _lo.u = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rs].u / (uint32_t)_reg[_cur_instr.rt].u);
        _hi.u = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rs].u % (uint32_t)_reg[_cur_instr.rt].u);
    }
    else
    {
        LOG_WARNING("DIVU: divide by 0");
    }
    ++_PC;
}

void Interpreter::DMULT(void)
{
#if defined(__GNUC__)
    __int128_t rs128 = (int64_t)_reg[_cur_instr.rs].s;
    __int128_t rs128 = (int64_t)_reg[_cur_instr.rt].s;
    __int128_t result = rs128 * rs128;

    _lo.s = result;
    _hi.s = (result >> 64);
#elif defined(_MSC_VER)
    _lo.s = _mul128(_reg[_cur_instr.rs].s, _reg[_cur_instr.rt].s, &_hi.s);
#else
    int64_t hi_prod, mid_prods, lo_prod;
    int64_t rshi = (int32_t)(_reg[_cur_instr.rs].s >> 32);
    int64_t rthi = (int32_t)(_reg[_cur_instr.rt].s >> 32);
    int64_t rslo = (uint32_t)_reg[_cur_instr.rs].u;
    int64_t rtlo = (uint32_t)_reg[_cur_instr.rt].u;

    mid_prods = (rshi * rtlo) + (rslo * rthi);
    lo_prod = (rslo * rtlo);
    hi_prod = (rshi * rthi);

    mid_prods += lo_prod >> 32;
    _hi.s = hi_prod + (mid_prods >> 32);
    _lo.s = (uint32_t)lo_prod + (mid_prods << 32);
#endif

    ++_PC;
}

void Interpreter::DMULTU(void)
{
#if defined(__GNUC__)
    __uint128_t rs128 = _reg[_cur_instr.rs].u;
    __uint128_t rs128 = _reg[_cur_instr.rt].u;

    __uint128_t result = rs128 * rt128;
    _lo.u = result;
    _hi.u = (result >> 64);
#elif defined(_MSC_VER)
    _lo.u = _umul128(_reg[_cur_instr.rs].u, _reg[_cur_instr.rt].u, &_hi.u);
#else
    uint64_t hi_prod, mid_prods, lo_prod;
    uint64_t rshi = (int32_t) (_reg[_cur_instr.rs].s >> 32);
    uint64_t rthi = (int32_t) (_reg[_cur_instr.rt].s >> 32);
    uint64_t rslo = (uint32_t) _reg[_cur_instr.rs].u;
    uint64_t rtlo = (uint32_t) _reg[_cur_instr.rt].u;

    mid_prods = (rshi * rtlo) + (rslo * rthi);
    lo_prod = (rslo * rtlo);
    hi_prod = (rshi * rthi);

    mid_prods += lo_prod >> 32;
    _hi.u = hi_prod + (mid_prods >> 32);
    _lo.u = (uint32_t)lo_prod + (mid_prods << 32);
#endif

    ++_PC;
}

void Interpreter::DDIV(void)
{
    if (_reg[_cur_instr.rt].u != 0)
    {
        _lo.s = _reg[_cur_instr.rs].s / _reg[_cur_instr.rt].s;
        _hi.s = _reg[_cur_instr.rs].s % _reg[_cur_instr.rt].s;
    }
    else
    {
        LOG_WARNING("DDIV: divide by 0");
    }
    ++_PC;
}

void Interpreter::DDIVU(void)
{
    if (_reg[_cur_instr.rt].u != 0)
    {
        _lo.u = _reg[_cur_instr.rs].u / _reg[_cur_instr.rt].u;
        _hi.u = _reg[_cur_instr.rs].u % _reg[_cur_instr.rt].u;
    }
    else
    {
        LOG_WARNING("DDIVU: divide by 0");
    }
    ++_PC;
}

void Interpreter::ADD(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void Interpreter::ADDU(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void Interpreter::SUB(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s - (int32_t)_reg[_cur_instr.rt].s);

    ++_PC;
}

void Interpreter::SUBU(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s - (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void Interpreter::AND(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s & _reg[_cur_instr.rt].s;
    }

    ++_PC;
}

void Interpreter::OR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s | _reg[_cur_instr.rt].s;
    }
    
    ++_PC;
}

void Interpreter::XOR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s ^ _reg[_cur_instr.rt].s;
    }
    
    ++_PC;
}

void Interpreter::NOR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = ~(_reg[_cur_instr.rs].s | _reg[_cur_instr.rt].s);
    }

    ++_PC;
}

void Interpreter::SLT(void)
{
    if (_reg[_cur_instr.rs].s < _reg[_cur_instr.rt].s)
    {
        _reg[_cur_instr.rd].s = 1;
    }
    else
    {
        _reg[_cur_instr.rd].s = 0;
    }

    ++_PC;
}

void Interpreter::SLTU(void)
{
    if (_reg[_cur_instr.rs].u < _reg[_cur_instr.rt].u)
    {
        _reg[_cur_instr.rd].s = 1;
    }
    else
    {
        _reg[_cur_instr.rd].s = 0;
    }

    ++_PC;
}

void Interpreter::DADD(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s + _reg[_cur_instr.rt].s;

    ++_PC;
}

void Interpreter::DADDU(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s + _reg[_cur_instr.rt].s;

    ++_PC;
}

void Interpreter::DSUB(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s - _reg[_cur_instr.rt].s;

    ++_PC;
}

void Interpreter::DSUBU(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s - _reg[_cur_instr.rt].s;

    ++_PC;
}

void Interpreter::TGE(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TGEU(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TLT(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TLTU(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TEQ(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::TNE(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::DSLL(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s << _cur_instr.sa);

    ++_PC;
}

void Interpreter::DSRL(void)
{
    _reg[_cur_instr.rd].u = (_reg[_cur_instr.rt].u >> _cur_instr.sa);

    ++_PC;
}

void Interpreter::DSRA(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s >> _cur_instr.sa);

    ++_PC;
}

void Interpreter::DSLL32(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s << (32 + _cur_instr.sa));

    ++_PC;
}

void Interpreter::DSRL32(void)
{
    _reg[_cur_instr.rd].u = (_reg[_cur_instr.rt].u >> (32 + _cur_instr.sa));

    ++_PC;
}

void Interpreter::DSRA32(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s >> (32 + _cur_instr.sa));

    ++_PC;
}