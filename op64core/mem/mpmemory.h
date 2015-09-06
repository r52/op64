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

private:
    memptr_read readmem_table[0x10000];
    memptr_write writemem_table[0x10000];
};
