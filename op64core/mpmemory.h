/* mupen64plus' implementation of memory */
#pragma once

#include <cstdint>
#include "imemory.h"
#include "bus.h"
#include "tlb.h"


class MPMemory : public IMemory
{

    typedef void(MPMemory::*memptr_read)(uint32_t&, uint64_t*, DataSize);
    typedef void(MPMemory::*memptr_write)(uint32_t, uint64_t, DataSize);

    typedef void(MPMemory::*readfn)(uint32_t, uint32_t*);
    typedef void(MPMemory::*writefn)(uint32_t, uint32_t, uint32_t);

    typedef void(MPMemory::*dataptr_read)(readfn, uint32_t&, uint64_t*);
    typedef void(MPMemory::*dataptr_write)(writefn, uint32_t, uint64_t);

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

private:
    // Data read
    void read_size_byte(readfn read_func, uint32_t& address, uint64_t* dest);
    void read_size_half(readfn read_func, uint32_t& address, uint64_t* dest);
    void read_size_word(readfn read_func, uint32_t& address, uint64_t* dest);
    void read_size_dword(readfn read_func, uint32_t& address, uint64_t* dest);

    // Data write
    void write_size_byte(writefn write_func, uint32_t address, uint64_t src);
    void write_size_half(writefn write_func, uint32_t address, uint64_t src);
    void write_size_word(writefn write_func, uint32_t address, uint64_t src);
    void write_size_dword(writefn write_func, uint32_t address, uint64_t src);

    // Read functions
    void read_nothing(uint32_t& address, uint64_t* dest, DataSize size);
    void read_nomem(uint32_t& address, uint64_t* dest, DataSize size);

    void read_rdram(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdram_func(uint32_t address, uint32_t* dest);
    
    void read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdram_reg_func(uint32_t address, uint32_t* dest);

    void read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_mem_func(uint32_t address, uint32_t* dest);

    void read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_reg_func(uint32_t address, uint32_t* dest);

    void read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rsp_stat_func(uint32_t address, uint32_t* dest);

    void read_dp(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dp_func(uint32_t address, uint32_t* dest);

    void read_dps(uint32_t& address, uint64_t* dest, DataSize size);
    void read_dps_func(uint32_t address, uint32_t* dest);

    void read_mi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_mi_func(uint32_t address, uint32_t* dest);

    void read_vi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_vi_func(uint32_t address, uint32_t* dest);

    void read_ai(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ai_func(uint32_t address, uint32_t* dest);

    void read_pi(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pi_func(uint32_t address, uint32_t* dest);

    void read_ri(uint32_t& address, uint64_t* dest, DataSize size);
    void read_ri_func(uint32_t address, uint32_t* dest);

    void read_si(uint32_t& address, uint64_t* dest, DataSize size);
    void read_si_func(uint32_t address, uint32_t* dest);

    void read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size);
    void read_flashram_status_func(uint32_t address, uint32_t* dest);

    void read_rom(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rom_func(uint32_t address, uint32_t* dest);

    void read_pif(uint32_t& address, uint64_t* dest, DataSize size);
    void read_pif_func(uint32_t address, uint32_t* dest);

    void read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdramFB_func(uint32_t address, uint32_t* dest);

    // Write functions
    void write_nothing(uint32_t address, uint64_t src, DataSize size);
    void write_nomem(uint32_t address, uint64_t src, DataSize size);

    void write_rdram_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rdram(uint32_t address, uint64_t src, DataSize size);
    
    void write_rdram_reg_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rdram_reg(uint32_t address, uint64_t src, DataSize size);

    void write_rsp_mem_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rsp_mem(uint32_t address, uint64_t src, DataSize size);

    void write_rsp_reg_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rsp_reg(uint32_t address, uint64_t src, DataSize size);

    void write_rsp_stat_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rsp_stat(uint32_t address, uint64_t src, DataSize size);

    void write_dp_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_dp(uint32_t address, uint64_t src, DataSize size);

    void write_dps_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_dps(uint32_t address, uint64_t src, DataSize size);

    void write_mi_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_mi(uint32_t address, uint64_t src, DataSize size);

    void write_vi_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_vi(uint32_t address, uint64_t src, DataSize size);

    void write_ai_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_ai(uint32_t address, uint64_t src, DataSize size);

    void write_pi_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_pi(uint32_t address, uint64_t src, DataSize size);

    void write_ri_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_ri(uint32_t address, uint64_t src, DataSize size);

    void write_si_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_si(uint32_t address, uint64_t src, DataSize size);

    void write_flashram_dummy(uint32_t address, uint64_t src, DataSize size);

    void write_flashram_command_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_flashram_command(uint32_t address, uint64_t src, DataSize size);

    void write_rom_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rom(uint32_t address, uint64_t src, DataSize size);

    void write_pif_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_pif(uint32_t address, uint64_t src, DataSize size);

    void write_rdramFB_func(uint32_t address, uint32_t data, uint32_t mask);
    void write_rdramFB(uint32_t address, uint64_t src, DataSize size);

    // helpers
    bool update_mi_init_mode(uint32_t w);
    void update_mi_intr_mask(uint32_t w);
    void update_sp_reg(uint32_t w);
    void prepare_rsp(void);
    bool updateDPC(uint32_t w);

private:

    dataptr_read readsize[NUM_DATA_SIZES];
    dataptr_write writesize[NUM_DATA_SIZES];

    memptr_read readmem_table[0x10000];
    memptr_write writemem_table[0x10000];

    uint32_t _rom_lastwrite = 0;
};
