#pragma once

#include "rcpcommon.h"
#include "util.h"

enum DataSize
{
    SIZE_WORD = 0,
    SIZE_DWORD = 1,
    SIZE_HWORD = 2,
    SIZE_BYTE = 3
};

class IMemory
{

public:
    ~IMemory();

    virtual void initialize(void);
    virtual void uninitialize(void);
    virtual void readmem(uint32_t& address, uint64_t* dest, DataSize size) = 0;
    virtual void writemem(uint32_t address, uint64_t src, DataSize size) = 0;

    // shouldn't be using this except with mupen interpreter
    virtual uint32_t* fast_fetch(uint32_t address) = 0;

protected:
    IMemory();

    uint32_t _SP_MEM[0x1000 / 4 * 2];    // allocate exactly 2048 * (4 byte uint32) = 8192 bytes (8KB) of SP MEM
    uint32_t* const _SP_DMEM = _SP_MEM;
    uint32_t* const _SP_IMEM = _SP_DMEM + (0x1000 / 4);     // IMEM is 4KB (0x400 = 1024 4 byte ints) into the memory

    __align(uint32_t _rdram[0x800000 / 4], 16);
    uint32_t _PIF_RAM[0x40 / 4];

    uint32_t _vi_reg[VI_NUM_REGS];
    uint32_t _mi_reg[MI_NUM_REGS];
    uint32_t _rdram_reg[RDRAM_NUM_REGS];
    uint32_t _sp_reg[SP_NUM_REGS];
    uint32_t _ai_reg[AI_NUM_REGS];
    uint32_t _pi_reg[PI_NUM_REGS];
    uint32_t _ri_reg[RI_NUM_REGS];
    uint32_t _si_reg[SI_NUM_REGS];
    uint32_t _dp_reg[DPC_NUM_REGS];
    uint32_t _dps_reg[DPS_NUM_REGS];
};
