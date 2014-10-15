#include "mppinterpreter.h"
#include "logger.h"
#include "bus.h"
#include "cp1.h"
#include "fpu.h"


void MPPInterpreter::MFC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    _reg[_cur_instr.rt].s = *(int32_t*)Bus::s_reg[_cur_instr.fs];
    ++_PC;
}

void MPPInterpreter::DMFC1(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CFC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    if (_cur_instr.fs == 31)
    {
        _reg[_cur_instr.rt].s = *(int32_t*)Bus::FCR31;
    }

    if (_cur_instr.fs == 0)
    {
        _reg[_cur_instr.rt].s = *(int32_t*)Bus::FCR0;
    }
    ++_PC;
}

void MPPInterpreter::MTC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    *((int32_t*)Bus::s_reg[_cur_instr.fs]) = (int32_t)_reg[_cur_instr.rt].s;
    ++_PC;
}

void MPPInterpreter::DMTC1(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CTC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    if (_cur_instr.fs == 31)
        *(Bus::FCR31) = (int32_t)_reg[_cur_instr.rt].s;

    switch ((*(Bus::FCR31) & 3))
    {
    case 0:
        _cp1->rounding_mode = ROUND_MODE; // Round to nearest, or to even if equidistant
        break;
    case 1:
        _cp1->rounding_mode = TRUNC_MODE; // Truncate (toward 0)
        break;
    case 2:
        _cp1->rounding_mode = CEIL_MODE; // Round up (toward +infinity) 
        break;
    case 3:
        _cp1->rounding_mode = FLOOR_MODE; // Round down (toward -infinity) 
        break;
    }
    //if ((FCR31 >> 7) & 0x1F) printf("FPU Exception enabled : %x\n",
    //                 (int)((FCR31 >> 7) & 0x1F));
    ++_PC;
}

void MPPInterpreter::BC(void)
{
    (this->*bc_table[_cur_instr.ft])();
}

void MPPInterpreter::S(void)
{
    (this->*s_table[_cur_instr.func])();
}

void MPPInterpreter::D(void)
{
    (this->*d_table[_cur_instr.func])();
}

void MPPInterpreter::W(void)
{
    (this->*w_table[_cur_instr.func])();
}

void MPPInterpreter::L(void)
{
    (this->*l_table[_cur_instr.func])();
}

void MPPInterpreter::BC1F(void)
{
    // DECLARE_JUMP(BC1F,  PCADDR + (iimmediate+1)*4, (FCR31 & 0x800000)==0, &reg[0], 0, 1)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        (*(Bus::FCR31) & 0x800000) == 0,
        &_reg[0],
        false,
        true
        );
}

void MPPInterpreter::BC1T(void)
{
    // DECLARE_JUMP(BC1T,  PCADDR + (iimmediate+1)*4, (FCR31 & 0x800000)!=0, &reg[0], 0, 1)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        (*(Bus::FCR31) & 0x800000) != 0,
        &_reg[0],
        false,
        true
        );
}

void MPPInterpreter::BC1FL(void)
{
    // DECLARE_JUMP(BC1FL, PCADDR + (iimmediate+1)*4, (FCR31 & 0x800000)==0, &reg[0], 1, 1)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        (*(Bus::FCR31) & 0x800000) == 0,
        &_reg[0],
        true,
        true
        );
}

void MPPInterpreter::BC1TL(void)
{
    // DECLARE_JUMP(BC1TL, PCADDR + (iimmediate+1)*4, (FCR31 & 0x800000)!=0, &reg[0], 1, 1)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        (*(Bus::FCR31) & 0x800000) != 0,
        &_reg[0],
        true,
        true
        );
}

void MPPInterpreter::ADD_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = add_f32((float*)Bus::s_reg[_cur_instr.fs], (float*)Bus::s_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::SUB_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = sub_f32((float*)Bus::s_reg[_cur_instr.fs], (float*)Bus::s_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::MUL_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = mul_f32((float*)Bus::s_reg[_cur_instr.fs], (float*)Bus::s_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::DIV_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    if ((*Bus::FCR31 & 0x400) && *Bus::s_reg[_cur_instr.ft] == 0)
    {
        LOG_ERROR("DIV_S by 0");
    }

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = div_f32((float*)Bus::s_reg[_cur_instr.fs], (float*)Bus::s_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::SQRT_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = sqrt_f32((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::ABS_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = abs_f32((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::MOV_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = *(float*)Bus::s_reg[_cur_instr.fs];

    ++_PC;
}

void MPPInterpreter::NEG_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(float*)Bus::s_reg[_cur_instr.fd] = neg_f32((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::ROUND_L_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TRUNC_L_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CEIL_L_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::FLOOR_L_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::ROUND_W_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TRUNC_W_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint32_t saved_mode = get_rounding();

    set_rounding(ROUND_MODE);
    *(int32_t*)Bus::s_reg[_cur_instr.fd] = trunc_f32_to_i32((float*)Bus::s_reg[_cur_instr.fs]);
    set_rounding(saved_mode);

    ++_PC;
}

void MPPInterpreter::CEIL_W_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::FLOOR_W_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CVT_D_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(double*)Bus::d_reg[_cur_instr.fd] = f32_to_f64((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_W_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(int32_t*)Bus::s_reg[_cur_instr.fd] = f32_to_i32((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_L_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(int64_t*)Bus::d_reg[_cur_instr.fd] = f32_to_i64((float*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::C_F_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_UN_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_EQ_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;
    /*
    if (isnan(*Bus::s_reg[_cur_instr.fs]) || isnan(*Bus::s_reg[_cur_instr.ft]))
    {
        *(Bus::FCR31) &= ~0x800000;
        return;
    }

    *(Bus::FCR31) = (*Bus::s_reg[_cur_instr.fs] == *Bus::s_reg[_cur_instr.ft]) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;
    */

    uint8_t result = c_cmp_32(Bus::s_reg[_cur_instr.fs], Bus::s_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        *(Bus::FCR31) &= ~0x800000;
        return;
    }

    *(Bus::FCR31) = (result == CMP_EQUAL) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;
}

void MPPInterpreter::C_UEQ_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_OLT_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_ULT_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_OLE_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_ULE_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_SF_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_NGLE_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_SEQ_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_NGL_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_LT_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint8_t result = c_cmp_32(Bus::s_reg[_cur_instr.fs], Bus::s_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        LOG_ERROR("NaN in %s", __FUNCTION__);
        Bus::stop = 1;
    }

    *(Bus::FCR31) = (result == CMP_LESS_THAN) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;

    ++_PC;
}

void MPPInterpreter::C_NGE_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_LE_S(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint8_t result = c_cmp_32(Bus::s_reg[_cur_instr.fs], Bus::s_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        LOG_ERROR("NaN in %s", __FUNCTION__);
        Bus::stop = 1;
    }

    *(Bus::FCR31) = ((result == CMP_LESS_THAN) || (result == CMP_EQUAL)) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;

    ++_PC;
}

void MPPInterpreter::C_NGT_S(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::ADD_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = add_f64((double*)Bus::d_reg[_cur_instr.fs], (double*)Bus::d_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::SUB_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = sub_f64((double*)Bus::d_reg[_cur_instr.fs], (double*)Bus::d_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::MUL_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = mul_f64((double*)Bus::d_reg[_cur_instr.fs], (double*)Bus::d_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::DIV_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    if ((*Bus::FCR31 & 0x400) && *Bus::d_reg[_cur_instr.ft] == 0)
    {
        LOG_ERROR("DIV_D by 0");
    }

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = div_f64((double*)Bus::d_reg[_cur_instr.fs], (double*)Bus::d_reg[_cur_instr.ft]);
    ++_PC;
}

void MPPInterpreter::SQRT_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = sqrt_f64((double*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::ABS_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *(double*)Bus::d_reg[_cur_instr.fd] = abs_f64((double*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::MOV_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((int64_t*)Bus::d_reg[_cur_instr.fd]) = *((int64_t*)Bus::d_reg[_cur_instr.fs]);

    ++_PC;
}

void MPPInterpreter::NEG_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    *((double*)Bus::d_reg[_cur_instr.fd]) = neg_f64((double*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::ROUND_L_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TRUNC_L_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CEIL_L_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::FLOOR_L_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::ROUND_W_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TRUNC_W_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint32_t saved_mode = get_rounding();
    set_rounding(ROUND_MODE);
    *(int32_t*)Bus::s_reg[_cur_instr.fd] = trunc_f64_to_i32((double*)Bus::d_reg[_cur_instr.fs]);
    set_rounding(saved_mode);

    ++_PC;
}

void MPPInterpreter::CEIL_W_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::FLOOR_W_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CVT_S_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((float*)Bus::s_reg[_cur_instr.fd]) = f64_to_f32((double*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_W_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((int32_t*)Bus::s_reg[_cur_instr.fd]) = f64_to_i32(*(double*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_L_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_F_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_UN_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_EQ_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;

    uint8_t result = c_cmp_64(Bus::d_reg[_cur_instr.fs], Bus::d_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        *(Bus::FCR31) &= ~0x800000;
        return;
    }

    *(Bus::FCR31) = (result == CMP_EQUAL) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;
}

void MPPInterpreter::C_UEQ_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_OLT_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_ULT_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_OLE_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_ULE_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_SF_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_NGLE_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_SEQ_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_NGL_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_LT_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint8_t result = c_cmp_64(Bus::d_reg[_cur_instr.fs], Bus::d_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        LOG_ERROR("NaN in %s", __FUNCTION__);
        Bus::stop = 1;
    }

    *(Bus::FCR31) = (result == CMP_LESS_THAN) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;

    ++_PC;
}

void MPPInterpreter::C_NGE_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::C_LE_D(void)
{
    if (_cp0->cop1_unusable())
        return;

    uint8_t result = c_cmp_64(Bus::d_reg[_cur_instr.fs], Bus::d_reg[_cur_instr.ft]);

    if (result == CMP_UNORDERED)
    {
        LOG_ERROR("NaN in %s", __FUNCTION__);
        Bus::stop = 1;
    }

    *(Bus::FCR31) = ((result == CMP_LESS_THAN) || (result == CMP_EQUAL)) ? *(Bus::FCR31) | 0x800000 : *(Bus::FCR31)&~0x800000;

    ++_PC;
}

void MPPInterpreter::C_NGT_D(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::CVT_S_W(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((float*)Bus::s_reg[_cur_instr.fd]) = i32_to_f32(*(int32_t*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_D_W(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = i32_to_f64(*(int32_t*)Bus::s_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_D_L(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((double*)Bus::d_reg[_cur_instr.fd]) = i64_to_f64(*(int64_t*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}

void MPPInterpreter::CVT_S_L(void)
{
    if (_cp0->cop1_unusable())
        return;

    set_rounding();
    *((float*)Bus::s_reg[_cur_instr.fd]) = i64_to_f32(*(int64_t*)Bus::d_reg[_cur_instr.fs]);
    ++_PC;
}
