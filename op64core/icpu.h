#pragma once

#include <cstdint>
#include <array>

#include "compiler.h"

#define PC_SIZE 4

// core types
enum
{
    CPU_INTERPRETER,
    CPU_CACHED,
    CPU_JIT
};

class CP0;
class CP1;

/************************************************************************/
/* Instruction holder                                                   */
/************************************************************************/
union Instruction
{
    uint32_t code;

    struct
    {
        uint32_t offset : 16;
        uint32_t rt : 5;
        uint32_t rs : 5;
        uint32_t op : 6;
    };

    struct
    { 
        uint32_t immediate : 16;
        uint32_t _dum2 : 5;
        uint32_t base : 5;
        uint32_t _dum1 : 6;
    };

    struct
    {
        uint32_t target : 26;
        uint32_t _dum3 : 6;
    };

    struct
    {
        uint32_t func : 6;
        uint32_t sa : 5;
        uint32_t rd : 5;
        uint32_t _dum4 : 16;
    };

    struct
    { 
        uint32_t _dum6 : 6;
        uint32_t fd : 5;
        uint32_t fs : 5;
        uint32_t ft : 5;
        uint32_t fmt : 5;
        uint32_t _dum5 : 6;
    };

};

/************************************************************************/
/* Register struct                                                      */
/************************************************************************/

union Register64
{
    uint64_t u;
    int64_t s;
};

/************************************************************************/
/* Program Counter class                                                */
/************************************************************************/
class ProgramCounter
{
public:
    inline ProgramCounter(const uint32_t& x) :
        _addr(x)
    {}

    inline ProgramCounter(const ProgramCounter& x) :
        _addr(x._addr)
    {}

    inline ProgramCounter& operator=(const uint32_t& x)
    {
        _addr = x;
        return *this;
    }

    inline ProgramCounter& operator=(uint32_t&& x)
    {
        _addr = std::move(x);
        return *this;
    }

    inline ProgramCounter& operator=(const ProgramCounter& x)
    {
        _addr = x._addr;
        return *this;
    }

    inline operator uint32_t()
    {
        return _addr;
    }


    inline ProgramCounter operator+(const uint32_t& x)
    {
        uint32_t result = _addr + (x * PC_SIZE);
        return ProgramCounter(result);
    }

    /*/
    ProgramCounter operator+(const ProgramCounter& x)
    {
        uint32_t result = _addr + x._addr;
        return ProgramCounter(result);
    }

    ProgramCounter operator-(const ProgramCounter& x)
    {
        uint32_t result = _addr - x._addr;
        return ProgramCounter(result);
    }
    */

    inline ProgramCounter& operator++()
    {
        _addr += PC_SIZE;
        return *this;
    }

    inline ProgramCounter operator++(int)
    {
        ProgramCounter tmp(*this);
        operator++();
        return tmp;
    }

    inline ProgramCounter& operator--()
    {
        _addr -= PC_SIZE;
        return *this;
    }

    inline ProgramCounter operator--(int)
    {
        ProgramCounter tmp(*this);
        operator--();
        return tmp;
    }

    inline ProgramCounter& operator+=(const uint32_t& x)
    {
        _addr += (x * PC_SIZE);
        return *this;
    }

    inline bool operator==(const ProgramCounter& x)
    {
        return (this->_addr == x._addr);
    }

private:
    uint_fast32_t _addr;
};


/************************************************************************/
/* CPU interface                                                        */
/************************************************************************/

class ICPU
{
    typedef void(ICPU::*instruction_ptr)(void);
    typedef std::array<instruction_ptr, 64> InstructionTable;

public:
    ~ICPU(void);
    virtual uint32_t getCPUType(void) = 0;

    virtual void initialize(void) = 0;
    virtual void execute(void) = 0;
    virtual void hard_reset(void) = 0;
    virtual void soft_reset(void) = 0;

    virtual void global_jump_to(uint32_t addr) = 0;

    // COP0
    virtual void general_exception(void) = 0;
    virtual void TLB_refill_exception(unsigned int address, int w) = 0;

    inline CP0* getcp0(void)
    {
        return _cp0;
    };

    // COP1
    inline CP1* getcp1(void)
    {
        return _cp1;
    };

    // delay slot rw
    inline bool inDelaySlot(void)
    {
        return _delay_slot;
    }

    void setDelaySlot(bool delay)
    {
        _delay_slot = delay;
    }

    // fpu rounding mode
    int32_t rounding_mode;

protected:
    ICPU(void);

protected:

    //===========================================================================
    // regular instructions

    inline void SPECIAL(void)
    {
        (this->*special_table[_cur_instr.func])();
    }

    inline void REGIMM(void)
    {
        (this->*regimm_table[_cur_instr.rt])();
    }

    virtual void J(void) = 0;
    virtual void JAL(void) = 0;
    virtual void BEQ(void) = 0;
    virtual void BNE(void) = 0;
    virtual void BLEZ(void) = 0;
    virtual void BGTZ(void) = 0;

    virtual void ADDI(void) = 0;
    virtual void ADDIU(void) = 0;
    virtual void SLTI(void) = 0;
    virtual void SLTIU(void) = 0;
    virtual void ANDI(void) = 0;
    virtual void ORI(void) = 0;
    virtual void XORI(void) = 0;
    virtual void LUI(void) = 0;

    inline void COP0(void)
    {
        (this->*cop0_table[_cur_instr.rs])();
    }

    inline void COP1(void)
    {
        (this->*cop1_table[_cur_instr.fmt])();
    }

    virtual void SV(void) = 0;
    virtual void BEQL(void) = 0;
    virtual void BNEL(void) = 0;
    virtual void BLEZL(void) = 0;
    virtual void BGTZL(void) = 0;

    virtual void DADDI(void) = 0;
    virtual void DADDIU(void) = 0;
    virtual void LDL(void) = 0;
    virtual void LDR(void) = 0;

    virtual void LB(void) = 0;
    virtual void LH(void) = 0;
    virtual void LWL(void) = 0;
    virtual void LW(void) = 0;
    virtual void LBU(void) = 0;
    virtual void LHU(void) = 0;
    virtual void LWR(void) = 0;
    virtual void LWU(void) = 0;

    virtual void SB(void) = 0;
    virtual void SH(void) = 0;
    virtual void SWL(void) = 0;
    virtual void SW(void) = 0;
    virtual void SDL(void) = 0;
    virtual void SDR(void) = 0;
    virtual void SWR(void) = 0;
    virtual void CACHE(void) = 0;

    virtual void LL(void) = 0;
    virtual void LWC1(void) = 0;
    virtual void LLD(void) = 0;
    virtual void LDC1(void) = 0;
    virtual void LD(void) = 0;

    virtual void SC(void) = 0;
    virtual void SWC1(void) = 0;
    virtual void SCD(void) = 0;
    virtual void SDC1(void) = 0;
    virtual void SD(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable instruction_table = 
    {{
        &ICPU::SPECIAL, &ICPU::REGIMM, &ICPU::J, &ICPU::JAL, &ICPU::BEQ, &ICPU::BNE, &ICPU::BLEZ, &ICPU::BGTZ,
        &ICPU::ADDI, &ICPU::ADDIU, &ICPU::SLTI, &ICPU::SLTIU, &ICPU::ANDI, &ICPU::ORI, &ICPU::XORI, &ICPU::LUI,
        &ICPU::COP0, &ICPU::COP1, &ICPU::SV, &ICPU::SV, &ICPU::BEQL, &ICPU::BNEL, &ICPU::BLEZL, &ICPU::BGTZL,
        &ICPU::DADDI, &ICPU::DADDIU, &ICPU::LDL, &ICPU::LDR, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::LB, &ICPU::LH, &ICPU::LWL, &ICPU::LW, &ICPU::LBU, &ICPU::LHU, &ICPU::LWR, &ICPU::LWU,
        &ICPU::SB, &ICPU::SH, &ICPU::SWL, &ICPU::SW, &ICPU::SDL, &ICPU::SDR, &ICPU::SWR, &ICPU::CACHE,
        &ICPU::LL, &ICPU::LWC1, &ICPU::SV, &ICPU::SV, &ICPU::LLD, &ICPU::LDC1, &ICPU::SV, &ICPU::LD,
        &ICPU::SC, &ICPU::SWC1, &ICPU::SV, &ICPU::SV, &ICPU::SCD, &ICPU::SDC1, &ICPU::SV, &ICPU::SD
    }};
#else
    InstructionTable instruction_table;
#endif


    //===========================================================================
    // special instructions


    virtual void SLL(void) = 0;
    virtual void SRL(void) = 0;
    virtual void SRA(void) = 0;
    virtual void SLLV(void) = 0;
    virtual void SRLV(void) = 0;
    virtual void SRAV(void) = 0;

    virtual void JR(void) = 0;
    virtual void JALR(void) = 0;
    virtual void SYSCALL(void) = 0;
    virtual void BREAK(void) = 0;
    virtual void SYNC(void) = 0;

    virtual void MFHI(void) = 0;
    virtual void MTHI(void) = 0;
    virtual void MFLO(void) = 0;
    virtual void MTLO(void) = 0;
    virtual void DSLLV(void) = 0;
    virtual void DSRLV(void) = 0;
    virtual void DSRAV(void) = 0;

    virtual void MULT(void) = 0;
    virtual void MULTU(void) = 0;
    virtual void DIV(void) = 0;
    virtual void DIVU(void) = 0;
    virtual void DMULT(void) = 0;
    virtual void DMULTU(void) = 0;
    virtual void DDIV(void) = 0;
    virtual void DDIVU(void) = 0;

    virtual void ADD(void) = 0;
    virtual void ADDU(void) = 0;
    virtual void SUB(void) = 0;
    virtual void SUBU(void) = 0;
    virtual void AND(void) = 0;
    virtual void OR(void) = 0;
    virtual void XOR(void) = 0;
    virtual void NOR(void) = 0;

    virtual void SLT(void) = 0;
    virtual void SLTU(void) = 0;
    virtual void DADD(void) = 0;
    virtual void DADDU(void) = 0;
    virtual void DSUB(void) = 0;
    virtual void DSUBU(void) = 0;

    virtual void TGE(void) = 0;
    virtual void TGEU(void) = 0;
    virtual void TLT(void) = 0;
    virtual void TLTU(void) = 0;
    virtual void TEQ(void) = 0;
    virtual void TNE(void) = 0;

    virtual void DSLL(void) = 0;
    virtual void DSRL(void) = 0;
    virtual void DSRA(void) = 0;
    virtual void DSLL32(void) = 0;
    virtual void DSRL32(void) = 0;
    virtual void DSRA32(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable special_table = 
    {{
        &ICPU::SLL, &ICPU::SV, &ICPU::SRL, &ICPU::SRA, &ICPU::SLLV, &ICPU::SV, &ICPU::SRLV, &ICPU::SRAV,
        &ICPU::JR, &ICPU::JALR, &ICPU::SV, &ICPU::SV, &ICPU::SYSCALL, &ICPU::BREAK, &ICPU::SV, &ICPU::SYNC,
        &ICPU::MFHI, &ICPU::MTHI, &ICPU::MFLO, &ICPU::MTLO, &ICPU::DSLLV, &ICPU::SV, &ICPU::DSRLV, &ICPU::DSRAV,
        &ICPU::MULT, &ICPU::MULTU, &ICPU::DIV, &ICPU::DIVU, &ICPU::DMULT, &ICPU::DMULTU, &ICPU::DDIV, &ICPU::DDIVU,
        &ICPU::ADD, &ICPU::ADDU, &ICPU::SUB, &ICPU::SUBU, &ICPU::AND, &ICPU::OR, &ICPU::XOR, &ICPU::NOR,
        &ICPU::SV, &ICPU::SV, &ICPU::SLT, &ICPU::SLTU, &ICPU::DADD, &ICPU::DADDU, &ICPU::DSUB, &ICPU::DSUBU,
        &ICPU::TGE, &ICPU::TGEU, &ICPU::TLT, &ICPU::TLTU, &ICPU::TEQ, &ICPU::SV, &ICPU::TNE, &ICPU::SV,
        &ICPU::DSLL, &ICPU::SV, &ICPU::DSRL, &ICPU::DSRA, &ICPU::DSLL32, &ICPU::SV, &ICPU::DSRL32, &ICPU::DSRA32
    }};
#else
    InstructionTable special_table;
#endif


    //===========================================================================
    // regimm instructions

    virtual void BLTZ(void) = 0;
    virtual void BGEZ(void) = 0;
    virtual void BLTZL(void) = 0;
    virtual void BGEZL(void) = 0;
    virtual void TGEI(void) = 0;
    virtual void TGEIU(void) = 0;
    virtual void TLTI(void) = 0;
    virtual void TLTIU(void) = 0;
    virtual void TEQI(void) = 0;
    virtual void TNEI(void) = 0;
    virtual void BLTZAL(void) = 0;
    virtual void BGEZAL(void) = 0;
    virtual void BLTZALL(void) = 0;
    virtual void BGEZALL(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable regimm_table = 
    {{
        &ICPU::BLTZ, &ICPU::BGEZ, &ICPU::BLTZL, &ICPU::BGEZL, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::TGEI, &ICPU::TGEIU, &ICPU::TLTI, &ICPU::TLTIU, &ICPU::TEQI, &ICPU::SV, &ICPU::TNEI, &ICPU::SV,
        &ICPU::BLTZAL, &ICPU::BGEZAL, &ICPU::BLTZALL, &ICPU::BGEZALL, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
    }};
#else
    InstructionTable regimm_table;
#endif


    //===========================================================================
    // cop0 instructions

    virtual void MFC0(void) = 0;
    virtual void MTC0(void) = 0;

    inline void TLB(void)
    {
        (this->*tlb_table[_cur_instr.func])();
    }

#ifdef HAS_CXX11_LIST_INST
    InstructionTable cop0_table =
    {{
        &ICPU::MFC0, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::MTC0, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::TLB, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
    }};
#else
    InstructionTable cop0_table;
#endif


    //===========================================================================
    // cop1 instructions

    virtual void MFC1(void) = 0;
    virtual void DMFC1(void) = 0;
    virtual void CFC1(void) = 0;
    virtual void MTC1(void) = 0;
    virtual void DMTC1(void) = 0;
    virtual void CTC1(void) = 0;

    inline void BC(void)
    {
        (this->*bc_table[_cur_instr.ft])();
    }

    inline void S(void)
    {
        (this->*s_table[_cur_instr.func])();
    }

    inline void D(void)
    {
        (this->*d_table[_cur_instr.func])();
    }

    inline void W(void)
    {
        (this->*w_table[_cur_instr.func])();
    }

    inline void L(void)
    {
        (this->*l_table[_cur_instr.func])();
    }

#ifdef HAS_CXX11_LIST_INST
    InstructionTable cop1_table =
    {{
        &ICPU::MFC1,&ICPU::DMFC1,&ICPU::CFC1,&ICPU::SV,&ICPU::MTC1,&ICPU::DMTC1,&ICPU::CTC1,&ICPU::SV,
        &ICPU::BC  ,&ICPU::SV   ,&ICPU::SV  ,&ICPU::SV,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV  ,&ICPU::SV,
        &ICPU::S   ,&ICPU::D    ,&ICPU::SV  ,&ICPU::SV,&ICPU::W   ,&ICPU::L    ,&ICPU::SV  ,&ICPU::SV,
        &ICPU::SV  ,&ICPU::SV   ,&ICPU::SV  ,&ICPU::SV,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV  ,&ICPU::SV
    }};
#else
    InstructionTable cop1_table;
#endif

    //===========================================================================
    // tlb instructions

    virtual void TLBR(void) = 0;
    virtual void TLBWI(void) = 0;
    virtual void TLBWR(void) = 0;
    virtual void TLBP(void) = 0;
    virtual void ERET(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable tlb_table =
    {{
        &ICPU::SV  ,&ICPU::TLBR,&ICPU::TLBWI,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::TLBWR,&ICPU::SV, 
        &ICPU::TLBP,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV   ,&ICPU::SV, 
        &ICPU::SV  ,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV   ,&ICPU::SV, 
        &ICPU::ERET,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV   ,&ICPU::SV, 
        &ICPU::SV  ,&ICPU::SV  ,&ICPU::SV   ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV   ,&ICPU::SV, 
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
    }};
#else
    InstructionTable tlb_table;
#endif

    //===========================================================================
    // bc instructions

    virtual void BC1F(void) = 0;
    virtual void BC1T(void) = 0;
    virtual void BC1FL(void) = 0;
    virtual void BC1TL(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable bc_table =
    {{
        &ICPU::BC1F ,&ICPU::BC1T ,
        &ICPU::BC1FL,&ICPU::BC1TL
    }};
#else
    InstructionTable bc_table;
#endif

    //===========================================================================
    // s instructions
    virtual void ADD_S(void) = 0;
    virtual void SUB_S(void) = 0;
    virtual void MUL_S(void) = 0;
    virtual void DIV_S(void) = 0;
    virtual void SQRT_S(void) = 0;
    virtual void ABS_S(void) = 0;
    virtual void MOV_S(void) = 0;
    virtual void NEG_S(void) = 0;
    virtual void ROUND_L_S(void) = 0;
    virtual void TRUNC_L_S(void) = 0;
    virtual void CEIL_L_S(void) = 0;
    virtual void FLOOR_L_S(void) = 0;
    virtual void ROUND_W_S(void) = 0;
    virtual void TRUNC_W_S(void) = 0;
    virtual void CEIL_W_S(void) = 0;
    virtual void FLOOR_W_S(void) = 0;
    virtual void CVT_D_S(void) = 0;
    virtual void CVT_W_S(void) = 0;
    virtual void CVT_L_S(void) = 0;
    virtual void C_F_S(void) = 0;
    virtual void C_UN_S(void) = 0;
    virtual void C_EQ_S(void) = 0;
    virtual void C_UEQ_S(void) = 0;
    virtual void C_OLT_S(void) = 0;
    virtual void C_ULT_S(void) = 0;
    virtual void C_OLE_S(void) = 0;
    virtual void C_ULE_S(void) = 0;
    virtual void C_SF_S(void) = 0;
    virtual void C_NGLE_S(void) = 0;
    virtual void C_SEQ_S(void) = 0;
    virtual void C_NGL_S(void) = 0;
    virtual void C_LT_S(void) = 0;
    virtual void C_NGE_S(void) = 0;
    virtual void C_LE_S(void) = 0;
    virtual void C_NGT_S(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable s_table =
    {{
        &ICPU::ADD_S    ,&ICPU::SUB_S    ,&ICPU::MUL_S   ,&ICPU::DIV_S    ,&ICPU::SQRT_S   ,&ICPU::ABS_S    ,&ICPU::MOV_S   ,&ICPU::NEG_S    , 
        &ICPU::ROUND_L_S,&ICPU::TRUNC_L_S,&ICPU::CEIL_L_S,&ICPU::FLOOR_L_S,&ICPU::ROUND_W_S,&ICPU::TRUNC_W_S,&ICPU::CEIL_W_S,&ICPU::FLOOR_W_S, 
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       , 
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       , 
        &ICPU::SV       ,&ICPU::CVT_D_S  ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::CVT_W_S  ,&ICPU::CVT_L_S  ,&ICPU::SV      ,&ICPU::SV       , 
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       , 
        &ICPU::C_F_S    ,&ICPU::C_UN_S   ,&ICPU::C_EQ_S  ,&ICPU::C_UEQ_S  ,&ICPU::C_OLT_S  ,&ICPU::C_ULT_S  ,&ICPU::C_OLE_S ,&ICPU::C_ULE_S  , 
        &ICPU::C_SF_S   ,&ICPU::C_NGLE_S ,&ICPU::C_SEQ_S ,&ICPU::C_NGL_S  ,&ICPU::C_LT_S   ,&ICPU::C_NGE_S  ,&ICPU::C_LE_S  ,&ICPU::C_NGT_S
    }};
#else
    InstructionTable s_table;
#endif

    //===========================================================================
    // d instructions

    virtual void ADD_D(void) = 0;
    virtual void SUB_D(void) = 0;
    virtual void MUL_D(void) = 0;
    virtual void DIV_D(void) = 0;
    virtual void SQRT_D(void) = 0;
    virtual void ABS_D(void) = 0;
    virtual void MOV_D(void) = 0;
    virtual void NEG_D(void) = 0;
    virtual void ROUND_L_D(void) = 0;
    virtual void TRUNC_L_D(void) = 0;
    virtual void CEIL_L_D(void) = 0;
    virtual void FLOOR_L_D(void) = 0;
    virtual void ROUND_W_D(void) = 0;
    virtual void TRUNC_W_D(void) = 0;
    virtual void CEIL_W_D(void) = 0;
    virtual void FLOOR_W_D(void) = 0;
    virtual void CVT_S_D(void) = 0;
    virtual void CVT_W_D(void) = 0;
    virtual void CVT_L_D(void) = 0;
    virtual void C_F_D(void) = 0;
    virtual void C_UN_D(void) = 0;
    virtual void C_EQ_D(void) = 0;
    virtual void C_UEQ_D(void) = 0;
    virtual void C_OLT_D(void) = 0;
    virtual void C_ULT_D(void) = 0;
    virtual void C_OLE_D(void) = 0;
    virtual void C_ULE_D(void) = 0;
    virtual void C_SF_D(void) = 0;
    virtual void C_NGLE_D(void) = 0;
    virtual void C_SEQ_D(void) = 0;
    virtual void C_NGL_D(void) = 0;
    virtual void C_LT_D(void) = 0;
    virtual void C_NGE_D(void) = 0;
    virtual void C_LE_D(void) = 0;
    virtual void C_NGT_D(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable d_table =
    {{
        &ICPU::ADD_D    ,&ICPU::SUB_D    ,&ICPU::MUL_D   ,&ICPU::DIV_D    ,&ICPU::SQRT_D   ,&ICPU::ABS_D    ,&ICPU::MOV_D   ,&ICPU::NEG_D    ,
        &ICPU::ROUND_L_D,&ICPU::TRUNC_L_D,&ICPU::CEIL_L_D,&ICPU::FLOOR_L_D,&ICPU::ROUND_W_D,&ICPU::TRUNC_W_D,&ICPU::CEIL_W_D,&ICPU::FLOOR_W_D,
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,
        &ICPU::CVT_S_D  ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::CVT_W_D  ,&ICPU::CVT_L_D  ,&ICPU::SV      ,&ICPU::SV       ,
        &ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV       ,&ICPU::SV      ,&ICPU::SV       ,
        &ICPU::C_F_D    ,&ICPU::C_UN_D   ,&ICPU::C_EQ_D  ,&ICPU::C_UEQ_D  ,&ICPU::C_OLT_D  ,&ICPU::C_ULT_D  ,&ICPU::C_OLE_D ,&ICPU::C_ULE_D  ,
        &ICPU::C_SF_D   ,&ICPU::C_NGLE_D ,&ICPU::C_SEQ_D ,&ICPU::C_NGL_D  ,&ICPU::C_LT_D   ,&ICPU::C_NGE_D  ,&ICPU::C_LE_D  ,&ICPU::C_NGT_D
    }};
#else
    InstructionTable d_table;
#endif

    //===========================================================================
    // w instructions

    virtual void CVT_S_W(void) = 0;
    virtual void CVT_D_W(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable w_table =
    {{
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::CVT_S_W, &ICPU::CVT_D_W, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
    }};
#else
    InstructionTable w_table;
#endif

    //===========================================================================
    // l instructions

    virtual void CVT_D_L(void) = 0;
    virtual void CVT_S_L(void) = 0;

#ifdef HAS_CXX11_LIST_INST
    InstructionTable l_table =
    {{
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV     ,&ICPU::SV     ,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV,&ICPU::SV, 
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::CVT_S_L, &ICPU::CVT_D_L, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV,
        &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV, &ICPU::SV
    }};
#else
    InstructionTable l_table;
#endif

protected:
    // PC
    ProgramCounter _PC;

    // current instruction data
    Instruction _cur_instr;

    // registers
    Register64 _reg[32];      // 32 64-bit general registers
    Register64 _hi;           // mult hi/lo registers
    Register64 _lo;

    // cp1 regs
    uint32_t _FCR0;
    uint32_t _FCR31;
    float* _s_reg[32];
    double* _d_reg[32];
    uint64_t _fgr[32];

    // COPs
    CP0* _cp0;
    CP1* _cp1;

    // LL bit
    bool _llbit;
    bool _delay_slot;
};
