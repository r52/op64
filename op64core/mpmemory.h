/* mupen64plus' implementation of memory */
#pragma once

#include <cstdint>
#include "imemory.h"
#include "bus.h"
#include "tlb.h"
#include "rcpinterface.h"


class MPMemory : public IMemory
{

    typedef void(MPMemory::*memptr_read)(uint32_t&, uint64_t*, DataSize);
    typedef void(MPMemory::*memptr_write)(uint32_t, uint64_t, DataSize);

    typedef void(MPMemory::*readfn)(uint32_t, uint32_t*);
    typedef void(MPMemory::*writefn)(uint32_t, uint32_t, uint32_t);

    typedef void(MPMemory::*dataptr_read)(RCPInterface&, uint32_t&, uint64_t*);
    typedef void(MPMemory::*dataptr_write)(RCPInterface&, uint32_t, uint64_t);

public:
    virtual void initialize(void);

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
    // Data read
    void read_size_byte(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_half(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_word(RCPInterface& device, uint32_t& address, uint64_t* dest);
    void read_size_dword(RCPInterface& device, uint32_t& address, uint64_t* dest);

    // Data write
    void write_size_byte(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_half(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_word(RCPInterface& device, uint32_t address, uint64_t src);
    void write_size_dword(RCPInterface& device, uint32_t address, uint64_t src);

    // Read functions
    void read_nothing(uint32_t& address, uint64_t* dest, DataSize size);
    void read_nomem(uint32_t& address, uint64_t* dest, DataSize size);

    void read_rdram(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dp(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dps(uint32_t& address, uint64_t* dest, DataSize size);
    void read_mi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_vi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ai(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ri(uint32_t& address, uint64_t* dest, DataSize size);
    void read_si(uint32_t& address, uint64_t* dest, DataSize size);

    void read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rom(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pif(uint32_t& address, uint64_t* dest, DataSize size);

    void read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size);

    // Write functions
    void write_nothing(uint32_t address, uint64_t src, DataSize size);
    void write_nomem(uint32_t address, uint64_t src, DataSize size);

    void write_rdram(uint32_t address, uint64_t src, DataSize size);
    void write_rdram_reg(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_mem(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_reg(uint32_t address, uint64_t src, DataSize size);
    void write_rsp_stat(uint32_t address, uint64_t src, DataSize size);
    void write_dp(uint32_t address, uint64_t src, DataSize size);
    void write_dps(uint32_t address, uint64_t src, DataSize size);
    void write_mi(uint32_t address, uint64_t src, DataSize size);
    void write_vi(uint32_t address, uint64_t src, DataSize size);
    void write_ai(uint32_t address, uint64_t src, DataSize size);
    void write_pi(uint32_t address, uint64_t src, DataSize size);
    void write_ri(uint32_t address, uint64_t src, DataSize size);
    void write_si(uint32_t address, uint64_t src, DataSize size);

    void write_flashram_dummy(uint32_t address, uint64_t src, DataSize size);
    void write_flashram_command(uint32_t address, uint64_t src, DataSize size);
    void write_rom(uint32_t address, uint64_t src, DataSize size);
    void write_pif(uint32_t address, uint64_t src, DataSize size);

    void write_rdramFB(uint32_t address, uint64_t src, DataSize size);

private:

    dataptr_read readsize[NUM_DATA_SIZES];
    dataptr_write writesize[NUM_DATA_SIZES];

    memptr_read readmem_table[0x10000];
    memptr_write writemem_table[0x10000];

    uint32_t _rom_lastwrite = 0;
};
