/* Interpreter implementation based on mupen64plus' pure interpreter */

#pragma once

#include <cstdint>

#include <oplog.h>

#include "icpu.h"

#include <core/bus.h>
#include <mem/imemory.h>


#define NOT_IMPLEMENTED() \
    Bus::stop = true; \
    LOG_ERROR(Interpreter) << "Not implemented. Stopping..."; \
    LOG_LEVEL(Interpreter, trace) << "PC: " << std::hex << (uint32_t)_PC << std::hex << _cur_instr.code;

#define DO_JUMP(target, condition, link, likely, cop1) \
    if (target == ((uint32_t)_PC) && _check_nop) \
    { \
        genericIdle(target, condition, link, likely, cop1); \
        return; \
    } \
    genericJump(target, condition, link, likely, cop1);


class Interpreter : public ICPU
{
public:
    Interpreter(void) = default;
    ~Interpreter(void);

    virtual uint32_t getCPUType(void)
    {
        return CPU_INTERPRETER;
    }

    virtual void initialize(void);
    virtual void execute(void);

    virtual void hardReset(void);
    virtual void softReset(void);

    virtual void globalJump(uint32_t addr);

    virtual void generalException(void);
    virtual void TLBRefillException(unsigned int address, TLBProbeMode mode, bool miss);

private:
    inline void prefetch(void)
    {
        uint32_t* mem = Bus::mem->fastFetch(_PC);

        if (nullptr == mem)
        {
            LOG_ERROR(Interpreter) << "Prefetch execution address " << std::hex << (uint32_t)_PC << " not found. Stopping...";
            Bus::stop = true;
            _cur_instr.code = 0;

            return;
        }

        _check_nop = (mem[1] == 0);
        _cur_instr.code = mem[0];
    }

private:
    // registers
    uint32_t* _cp0_reg;     // pointer to cp0 regs

    // cpu states
    bool _check_nop = false;
    bool _non_ieee_mode = true; // for testing

protected:

    void genericJump(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1);
    void genericIdle(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1);

    virtual void J(void);
    virtual void JAL(void);
    virtual void BEQ(void);
    virtual void BNE(void);
    virtual void BLEZ(void);
    virtual void BGTZ(void);
    virtual void ADDI(void);
    virtual void ADDIU(void);
    virtual void SLTI(void);
    virtual void SLTIU(void);
    virtual void ANDI(void);
    virtual void ORI(void);
    virtual void XORI(void);
    virtual void LUI(void);
    virtual void SV(void);
    virtual void BEQL(void);
    virtual void BNEL(void);
    virtual void BLEZL(void);
    virtual void BGTZL(void);
    virtual void DADDI(void);
    virtual void DADDIU(void);
    virtual void LDL(void);
    virtual void LDR(void);
    virtual void LB(void);
    virtual void LH(void);
    virtual void LWL(void);
    virtual void LW(void);
    virtual void LBU(void);
    virtual void LHU(void);
    virtual void LWR(void);
    virtual void LWU(void);
    virtual void SB(void);
    virtual void SH(void);
    virtual void SWL(void);
    virtual void SW(void);
    virtual void SDL(void);
    virtual void SDR(void);
    virtual void SWR(void);
    virtual void CACHE(void);
    virtual void LL(void);
    virtual void LWC1(void);
    virtual void LLD(void);
    virtual void LDC1(void);
    virtual void LD(void);
    virtual void SC(void);
    virtual void SWC1(void);
    virtual void SCD(void);
    virtual void SDC1(void);
    virtual void SD(void);

    virtual void SLL(void);
    virtual void SRL(void);
    virtual void SRA(void);
    virtual void SLLV(void);
    virtual void SRLV(void);
    virtual void SRAV(void);
    virtual void JR(void);
    virtual void JALR(void);
    virtual void SYSCALL(void);
    virtual void BREAK(void);
    virtual void SYNC(void);
    virtual void MFHI(void);
    virtual void MTHI(void);
    virtual void MFLO(void);
    virtual void MTLO(void);
    virtual void DSLLV(void);
    virtual void DSRLV(void);
    virtual void DSRAV(void);
    virtual void MULT(void);
    virtual void MULTU(void);
    virtual void DIV(void);
    virtual void DIVU(void);
    virtual void DMULT(void);
    virtual void DMULTU(void);
    virtual void DDIV(void);
    virtual void DDIVU(void);
    virtual void ADD(void);
    virtual void ADDU(void);
    virtual void SUB(void);
    virtual void SUBU(void);
    virtual void AND(void);
    virtual void OR(void);
    virtual void XOR(void);
    virtual void NOR(void);
    virtual void SLT(void);
    virtual void SLTU(void);
    virtual void DADD(void);
    virtual void DADDU(void);
    virtual void DSUB(void);
    virtual void DSUBU(void);
    virtual void TGE(void);
    virtual void TGEU(void);
    virtual void TLT(void);
    virtual void TLTU(void);
    virtual void TEQ(void);
    virtual void TNE(void);
    virtual void DSLL(void);
    virtual void DSRL(void);
    virtual void DSRA(void);
    virtual void DSLL32(void);
    virtual void DSRL32(void);
    virtual void DSRA32(void);

    virtual void BLTZ(void);
    virtual void BGEZ(void);
    virtual void BLTZL(void);
    virtual void BGEZL(void);
    virtual void TGEI(void);
    virtual void TGEIU(void);
    virtual void TLTI(void);
    virtual void TLTIU(void);
    virtual void TEQI(void);
    virtual void TNEI(void);
    virtual void BLTZAL(void);
    virtual void BGEZAL(void);
    virtual void BLTZALL(void);
    virtual void BGEZALL(void);

    virtual void MFC0(void);
    virtual void MTC0(void);

    virtual void MFC1(void);
    virtual void DMFC1(void);
    virtual void CFC1(void);
    virtual void MTC1(void);
    virtual void DMTC1(void);
    virtual void CTC1(void);

    virtual void TLBR(void);
    virtual void TLBWI(void);
    virtual void TLBWR(void);
    virtual void TLBP(void);
    virtual void ERET(void);

    virtual void BC1F(void);
    virtual void BC1T(void);
    virtual void BC1FL(void);
    virtual void BC1TL(void);

    virtual void ADD_S(void);
    virtual void SUB_S(void);
    virtual void MUL_S(void);
    virtual void DIV_S(void);
    virtual void SQRT_S(void);
    virtual void ABS_S(void);
    virtual void MOV_S(void);
    virtual void NEG_S(void);
    virtual void ROUND_L_S(void);
    virtual void TRUNC_L_S(void);
    virtual void CEIL_L_S(void);
    virtual void FLOOR_L_S(void);
    virtual void ROUND_W_S(void);
    virtual void TRUNC_W_S(void);
    virtual void CEIL_W_S(void);
    virtual void FLOOR_W_S(void);
    virtual void CVT_D_S(void);
    virtual void CVT_W_S(void);
    virtual void CVT_L_S(void);
    virtual void C_F_S(void);
    virtual void C_UN_S(void);
    virtual void C_EQ_S(void);
    virtual void C_UEQ_S(void);
    virtual void C_OLT_S(void);
    virtual void C_ULT_S(void);
    virtual void C_OLE_S(void);
    virtual void C_ULE_S(void);
    virtual void C_SF_S(void);
    virtual void C_NGLE_S(void);
    virtual void C_SEQ_S(void);
    virtual void C_NGL_S(void);
    virtual void C_LT_S(void);
    virtual void C_NGE_S(void);
    virtual void C_LE_S(void);
    virtual void C_NGT_S(void);

    virtual void ADD_D(void);
    virtual void SUB_D(void);
    virtual void MUL_D(void);
    virtual void DIV_D(void);
    virtual void SQRT_D(void);
    virtual void ABS_D(void);
    virtual void MOV_D(void);
    virtual void NEG_D(void);
    virtual void ROUND_L_D(void);
    virtual void TRUNC_L_D(void);
    virtual void CEIL_L_D(void);
    virtual void FLOOR_L_D(void);
    virtual void ROUND_W_D(void);
    virtual void TRUNC_W_D(void);
    virtual void CEIL_W_D(void);
    virtual void FLOOR_W_D(void);
    virtual void CVT_S_D(void);
    virtual void CVT_W_D(void);
    virtual void CVT_L_D(void);
    virtual void C_F_D(void);
    virtual void C_UN_D(void);
    virtual void C_EQ_D(void);
    virtual void C_UEQ_D(void);
    virtual void C_OLT_D(void);
    virtual void C_ULT_D(void);
    virtual void C_OLE_D(void);
    virtual void C_ULE_D(void);
    virtual void C_SF_D(void);
    virtual void C_NGLE_D(void);
    virtual void C_SEQ_D(void);
    virtual void C_NGL_D(void);
    virtual void C_LT_D(void);
    virtual void C_NGE_D(void);
    virtual void C_LE_D(void);
    virtual void C_NGT_D(void);

    virtual void CVT_S_W(void);
    virtual void CVT_D_W(void);

    virtual void CVT_D_L(void);
    virtual void CVT_S_L(void);

};
