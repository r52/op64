#include "mppinterpreter.h"
#include "util.h"
#include "logger.h"
#include "bus.h"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_umul128)
#pragma intrinsic(_mul128)
#endif

void MPPInterpreter::SLL(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>(((int32_t)_reg[_cur_instr.rt].s) << _cur_instr.sa);
    }
    
    ++_PC;
}

void MPPInterpreter::SRL(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rt].u >> _cur_instr.sa);
    }
    
    ++_PC;
}

void MPPInterpreter::SRA(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s >> _cur_instr.sa);
    }

    ++_PC;
}

void MPPInterpreter::SLLV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)(_reg[_cur_instr.rt].s) << ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void MPPInterpreter::SRLV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((uint32_t)_reg[_cur_instr.rt].u >> ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void MPPInterpreter::SRAV(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s >> ((uint32_t)_reg[_cur_instr.rs].u & 0x1F));
    }

    ++_PC;
}

void MPPInterpreter::JR(void)
{
    //DECLARE_JUMP(JR,   irs32, 1, &reg[0],    0, 0)

    generic_jump((uint32_t)_reg[_cur_instr.rs].u, true, &_reg[0], false, false);
}

void MPPInterpreter::JALR(void)
{
    // DECLARE_JUMP(JALR, irs32, 1, PC->f.r.rd, 0, 0)

    generic_jump((uint32_t)_reg[_cur_instr.rs].u, true, &_reg[_cur_instr.rd], false, false);
}

void MPPInterpreter::SYSCALL(void)
{
    _cp0_reg[CP0_CAUSE_REG] = 8 << 2;
    general_exception();
}

void MPPInterpreter::BREAK(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::SYNC(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::MFHI(void)
{
    _reg[_cur_instr.rd].s = _hi.s;

    ++_PC;
}

void MPPInterpreter::MTHI(void)
{
    _hi.s = _reg[_cur_instr.rs].s;
    ++_PC;
}

void MPPInterpreter::MFLO(void)
{
    _reg[_cur_instr.rd].s = _lo.s;
    
    ++_PC;
}

void MPPInterpreter::MTLO(void)
{
    _lo.s = _reg[_cur_instr.rs].s;
    ++_PC;
}

void MPPInterpreter::DSLLV(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rt].s << ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void MPPInterpreter::DSRLV(void)
{
    _reg[_cur_instr.rd].u = _reg[_cur_instr.rt].u >> ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void MPPInterpreter::DSRAV(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rt].s >> ((uint32_t)_reg[_cur_instr.rs].u & 0x3F);

    ++_PC;
}

void MPPInterpreter::MULT(void)
{
    int64_t result = (int64_t)((int32_t)_reg[_cur_instr.rs].s * (int64_t)((int32_t)_reg[_cur_instr.rt].s));
    _hi.s = signextend<int32_t, int64_t>(result >> 32);
    _lo.s = signextend<int32_t, int64_t>((int32_t)result);
    ++_PC;
}

void MPPInterpreter::MULTU(void)
{
    uint64_t result = (uint64_t)((uint32_t)_reg[_cur_instr.rs].u * (uint64_t)((uint32_t)_reg[_cur_instr.rt].u));
    _hi.s = signextend<int32_t, int64_t>(result >> 32);
    _lo.s = signextend<int32_t, int64_t>((int32_t)result);
    ++_PC;
}

void MPPInterpreter::DIV(void)
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

void MPPInterpreter::DIVU(void)
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

void MPPInterpreter::DMULT(void)
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

void MPPInterpreter::DMULTU(void)
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

void MPPInterpreter::DDIV(void)
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

void MPPInterpreter::DDIVU(void)
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

void MPPInterpreter::ADD(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void MPPInterpreter::ADDU(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void MPPInterpreter::SUB(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s - (int32_t)_reg[_cur_instr.rt].s);

    ++_PC;
}

void MPPInterpreter::SUBU(void)
{
    _reg[_cur_instr.rd].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s - (int32_t)_reg[_cur_instr.rt].s);
    
    ++_PC;
}

void MPPInterpreter::AND(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s & _reg[_cur_instr.rt].s;
    }

    ++_PC;
}

void MPPInterpreter::OR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s | _reg[_cur_instr.rt].s;
    }
    
    ++_PC;
}

void MPPInterpreter::XOR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s ^ _reg[_cur_instr.rt].s;
    }
    
    ++_PC;
}

void MPPInterpreter::NOR(void)
{
    if (_cur_instr.rd)
    {
        _reg[_cur_instr.rd].s = ~(_reg[_cur_instr.rs].s | _reg[_cur_instr.rt].s);
    }

    ++_PC;
}

void MPPInterpreter::SLT(void)
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

void MPPInterpreter::SLTU(void)
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

void MPPInterpreter::DADD(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s + _reg[_cur_instr.rt].s;

    ++_PC;
}

void MPPInterpreter::DADDU(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s + _reg[_cur_instr.rt].s;

    ++_PC;
}

void MPPInterpreter::DSUB(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s - _reg[_cur_instr.rt].s;

    ++_PC;
}

void MPPInterpreter::DSUBU(void)
{
    _reg[_cur_instr.rd].s = _reg[_cur_instr.rs].s - _reg[_cur_instr.rt].s;

    ++_PC;
}

void MPPInterpreter::TGE(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TGEU(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TLT(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TLTU(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TEQ(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TNE(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::DSLL(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s << _cur_instr.sa);

    ++_PC;
}

void MPPInterpreter::DSRL(void)
{
    _reg[_cur_instr.rd].u = (_reg[_cur_instr.rt].u >> _cur_instr.sa);

    ++_PC;
}

void MPPInterpreter::DSRA(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s >> _cur_instr.sa);

    ++_PC;
}

void MPPInterpreter::DSLL32(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s << (32 + _cur_instr.sa));

    ++_PC;
}

void MPPInterpreter::DSRL32(void)
{
    _reg[_cur_instr.rd].u = (_reg[_cur_instr.rt].u >> (32 + _cur_instr.sa));

    ++_PC;
}

void MPPInterpreter::DSRA32(void)
{
    _reg[_cur_instr.rd].s = (_reg[_cur_instr.rt].s >> (32 + _cur_instr.sa));

    ++_PC;
}