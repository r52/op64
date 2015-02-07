#pragma once

#include "rcpcommon.h"
#include "util.h"
#include "tlb.h"
#include "bus.h"

#define SP_MEM_SIZE 0x2000
#define RDRAM_SIZE 0x800000
#define PIF_RAM_SIZE 0x40

enum DataSize
{
    SIZE_WORD = 0,
    SIZE_DWORD = 1,
    SIZE_HWORD = 2,
    SIZE_BYTE = 3,
    NUM_DATA_SIZES
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
    inline uint32_t* fastFetch(uint32_t address)
    {
        if ((address & 0xc0000000) != 0x80000000)
        {
            address = TLB::virtual_to_physical_address(address, TLB_FAST_READ);
        }

        address &= 0x1ffffffc;

        if (address < RDRAM_SIZE)
        {
            return (uint32_t*)((uint8_t*)_rdram + address);
        }
        else if (address >= 0x10000000)
        {
            return (uint32_t*)((uint8_t*)Bus::rom_image + address - 0x10000000);
        }
        else if ((address & 0xffffe000) == 0x04000000)
        {
            return (uint32_t*)((uint8_t*)_SP_MEM + (address & 0x1ffc));
        }

        return nullptr;
    }

protected:
    IMemory();

    uint32_t _SP_MEM[SP_MEM_SIZE / 4];    // allocate exactly 2048 * (4 byte uint32) = 8192 bytes (8KB) of SP MEM
    uint32_t* const _SP_DMEM = _SP_MEM;
    uint32_t* const _SP_IMEM = _SP_DMEM + ((SP_MEM_SIZE/2) / 4);     // IMEM is 4KB (0x400 = 1024 4 byte ints) into the memory

    __align(uint32_t _rdram[RDRAM_SIZE / 4], 16);
    uint32_t _PIF_RAM[PIF_RAM_SIZE / 4];

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
