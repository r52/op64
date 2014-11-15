#include "icpu.h"
#include "bus.h"
#include "cp0.h"
#include "cp1.h"
#include "interrupthandler.h"


ICPU::ICPU(void) :
#ifndef HAS_CXX11_LIST_INST
instruction_table
({{
    &ICPU::SPECIAL, &ICPU::REGIMM, &ICPU::J, &ICPU::JAL, &ICPU::BEQ, &ICPU::BNE, &ICPU::BLEZ, &ICPU::BGTZ,
    &ICPU::ADDI, &ICPU::ADDIU, &ICPU::SLTI, &ICPU::SLTIU, &ICPU::ANDI, &ICPU::ORI, &ICPU::XORI, &ICPU::LUI,
    &ICPU::COP0, &ICPU::COP1, &ICPU::SV, &ICPU::SV, &ICPU::BEQL, &ICPU::BNEL, &ICPU::BLEZL, &ICPU::BGTZL,
    &ICPU::DADDI, &ICPU::DADDIU, &ICPU::LDL, &ICPU::LDR, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::LB, &ICPU::LH, &ICPU::LWL, &ICPU::LW, &ICPU::LBU, &ICPU::LHU, &ICPU::LWR, &ICPU::LWU,
    &ICPU::SB, &ICPU::SH, &ICPU::SWL, &ICPU::SW, &ICPU::SDL, &ICPU::SDR, &ICPU::SWR, &ICPU::CACHE,
    &ICPU::LL, &ICPU::LWC1, &ICPU::SV, &ICPU::SV, &ICPU::LLD, &ICPU::LDC1, &ICPU::SV, &ICPU::LD,
    &ICPU::SC, &ICPU::SWC1, &ICPU::SV, &ICPU::SV, &ICPU::SCD, &ICPU::SDC1, &ICPU::SV, &ICPU::SD
}}),
special_table
({{
    &ICPU::SLL, &ICPU::SV, &ICPU::SRL, &ICPU::SRA, &ICPU::SLLV, &ICPU::SV, &ICPU::SRLV, &ICPU::SRAV,
    &ICPU::JR, &ICPU::JALR, &ICPU::SV, &ICPU::SV, &ICPU::SYSCALL, &ICPU::BREAK, &ICPU::SV, &ICPU::SYNC,
    &ICPU::MFHI, &ICPU::MTHI, &ICPU::MFLO, &ICPU::MTLO, &ICPU::DSLLV, &ICPU::SV, &ICPU::DSRLV, &ICPU::DSRAV,
    &ICPU::MULT, &ICPU::MULTU, &ICPU::DIV, &ICPU::DIVU, &ICPU::DMULT, &ICPU::DMULTU, &ICPU::DDIV, &ICPU::DDIVU,
    &ICPU::ADD, &ICPU::ADDU, &ICPU::SUB, &ICPU::SUBU, &ICPU::AND, &ICPU::OR, &ICPU::XOR, &ICPU::NOR,
    &ICPU::SV, &ICPU::SV, &ICPU::SLT, &ICPU::SLTU, &ICPU::DADD, &ICPU::DADDU, &ICPU::DSUB, &ICPU::DSUBU,
    &ICPU::TGE, &ICPU::TGEU, &ICPU::TLT, &ICPU::TLTU, &ICPU::TEQ, &ICPU::SV, &ICPU::TNE, &ICPU::SV,
    &ICPU::DSLL, &ICPU::SV, &ICPU::DSRL, &ICPU::DSRA, &ICPU::DSLL32, &ICPU::SV, &ICPU::DSRL32, &ICPU::DSRA32
}}),
regimm_table
({{
    &ICPU::BLTZ, &ICPU::BGEZ, &ICPU::BLTZL, &ICPU::BGEZL, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::TGEI, &ICPU::TGEIU, &ICPU::TLTI, &ICPU::TLTIU, &ICPU::TEQI, &ICPU::SV, &ICPU::TNEI, &ICPU::SV,
    &ICPU::BLTZAL, &ICPU::BGEZAL, &ICPU::BLTZALL, &ICPU::BGEZALL, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
cop0_table
({{
    &ICPU::MFC0, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::MTC0, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::TLB, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
cop1_table
({{
    &ICPU::MFC1, &ICPU::DMFC1, &ICPU::CFC1, &ICPU::SV, &ICPU::MTC1, &ICPU::DMTC1, &ICPU::CTC1, &ICPU::SV,
    &ICPU::BC, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::S, &ICPU::D, &ICPU::SV, &ICPU::SV, &ICPU::W, &ICPU::L, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
tlb_table
({{
    &ICPU::SV, &ICPU::TLBR, &ICPU::TLBWI, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::TLBWR, &ICPU::SV,
    &ICPU::TLBP, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::ERET, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
bc_table
({{
    &ICPU::BC1F, &ICPU::BC1T,
    &ICPU::BC1FL, &ICPU::BC1TL
}}),
s_table
({{
    &ICPU::ADD_S, &ICPU::SUB_S, &ICPU::MUL_S, &ICPU::DIV_S, &ICPU::SQRT_S, &ICPU::ABS_S, &ICPU::MOV_S, &ICPU::NEG_S,
    &ICPU::ROUND_L_S, &ICPU::TRUNC_L_S, &ICPU::CEIL_L_S, &ICPU::FLOOR_L_S, &ICPU::ROUND_W_S, &ICPU::TRUNC_W_S, &ICPU::CEIL_W_S, &ICPU::FLOOR_W_S,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::CVT_D_S, &ICPU::SV, &ICPU::SV, &ICPU::CVT_W_S, &ICPU::CVT_L_S, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::C_F_S, &ICPU::C_UN_S, &ICPU::C_EQ_S, &ICPU::C_UEQ_S, &ICPU::C_OLT_S, &ICPU::C_ULT_S, &ICPU::C_OLE_S, &ICPU::C_ULE_S,
    &ICPU::C_SF_S, &ICPU::C_NGLE_S, &ICPU::C_SEQ_S, &ICPU::C_NGL_S, &ICPU::C_LT_S, &ICPU::C_NGE_S, &ICPU::C_LE_S, &ICPU::C_NGT_S
}}),
d_table
({{
    &ICPU::ADD_D, &ICPU::SUB_D, &ICPU::MUL_D, &ICPU::DIV_D, &ICPU::SQRT_D, &ICPU::ABS_D, &ICPU::MOV_D, &ICPU::NEG_D,
    &ICPU::ROUND_L_D, &ICPU::TRUNC_L_D, &ICPU::CEIL_L_D, &ICPU::FLOOR_L_D, &ICPU::ROUND_W_D, &ICPU::TRUNC_W_D, &ICPU::CEIL_W_D, &ICPU::FLOOR_W_D,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::CVT_S_D, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::CVT_W_D, &ICPU::CVT_L_D, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::C_F_D, &ICPU::C_UN_D, &ICPU::C_EQ_D, &ICPU::C_UEQ_D, &ICPU::C_OLT_D, &ICPU::C_ULT_D, &ICPU::C_OLE_D, &ICPU::C_ULE_D,
    &ICPU::C_SF_D, &ICPU::C_NGLE_D, &ICPU::C_SEQ_D, &ICPU::C_NGL_D, &ICPU::C_LT_D, &ICPU::C_NGE_D, &ICPU::C_LE_D, &ICPU::C_NGT_D
}}),
w_table
({{
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::CVT_S_W, &ICPU::CVT_D_W, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
l_table
({{
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::CVT_S_L, &ICPU::CVT_D_L, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
    &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
}}),
#endif
_PC(0),
_cur_instr({ 0 }),
_delay_slot(false),
_cp0(new CP0()),
_cp1(new CP1(&_s_reg, &_d_reg, &_fgr))
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

