/* mupen64plus' implementation of memory */
#pragma once

#include <cstdint>

#include "imemory.h"

class RCPInterface;

class MPMemory : public IMemory
{
    typedef void(IMemory::*memptr_read)(uint32_t&, uint64_t*, DataSize);
    typedef void(IMemory::*memptr_write)(uint32_t, uint64_t, DataSize);

public:
    virtual bool initialize(Bus* bus);

    inline virtual void readmem(uint32_t& address, uint64_t* dest, DataSize size) final
    {
        (this->*readmem_table[address >> 16])(address, dest, size);
    }

    inline virtual void writemem(uint32_t address, uint64_t src, DataSize size) final
    {
        (this->*writemem_table[address >> 16])(address, src, size);
    }

    virtual void unprotectFramebuffer(void);
    virtual void protectFramebuffer(void);

    virtual uint32_t* fetch(uint32_t address)
    {
        if ((address & 0xc0000000) != 0x80000000)
        {
            if (!(address = TLB::virtual_to_physical_address(_bus, address, TLB_FAST_READ)))
            {
                return nops;
            }
        }

        address &= 0x1ffffffc;

        if (address < RDRAM_SIZE)
        {
            return (uint32_t*)((uint8_t*)Bus::rdram.mem + address);
        }
        else if (address >= 0x10000000)
        {
            return (uint32_t*)((uint8_t*)_bus->rom->getImage() + address - 0x10000000);
        }
        else if ((address & 0xffffe000) == 0x04000000)
        {
            return (uint32_t*)((uint8_t*)Bus::rcp.sp.mem + (address & 0x1ffc));
        }

        return nullptr;
    }

private:
    memptr_read readmem_table[0x10000];
    memptr_write writemem_table[0x10000];
};
