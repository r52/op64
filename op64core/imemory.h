#pragma once

#include "rcpcommon.h"
#include "util.h"
#include "tlb.h"
#include "bus.h"
#include "rcp.h"
#include "rom.h"


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
            return (uint32_t*)((uint8_t*)Bus::rdram->mem + address);
        }
        else if (address >= 0x10000000)
        {
            return (uint32_t*)((uint8_t*)Bus::rom->getImage() + address - 0x10000000);
        }
        else if ((address & 0xffffe000) == 0x04000000)
        {
            return (uint32_t*)((uint8_t*)Bus::rcp->sp.mem + (address & 0x1ffc));
        }

        return nullptr;
    }

protected:
    IMemory();
};
