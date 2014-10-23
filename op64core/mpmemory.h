/* mupen64plus' implementation of memory */
#pragma once

#include <cstdint>
#include "imemory.h"


class MPMemory : public IMemory
{

    typedef void(MPMemory::*memptr)(uint32_t&, uint64_t*, DataSize);

public:
    virtual void initialize(void);

    inline virtual void readmem(uint32_t& address, uint64_t* dest, DataSize size) final
    {
        (this->*readmem_table[address >> 16])(address, dest, size);
    }

    inline virtual void writemem(uint32_t& address, uint64_t* src, DataSize size) final
    {
        (this->*writemem_table[address >> 16])(address, src, size);
    }

    virtual uint32_t* fast_fetch(uint32_t address);

private:

    // Read functions
    void read_nomem(uint32_t& address, uint64_t* dest, DataSize size);
    void read_rdram(uint32_t& address, uint64_t* dest, DataSize size);
    void read_nothing(uint32_t& address, uint64_t* dest, DataSize size);
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
    void write_nomem(uint32_t& address, uint64_t* src, DataSize size);
    void write_rdram(uint32_t& address, uint64_t* src, DataSize size);
    void write_nothing(uint32_t& address, uint64_t* src, DataSize size);
    void write_rdram_reg(uint32_t& address, uint64_t* src, DataSize size);
    void write_rsp_mem(uint32_t& address, uint64_t* src, DataSize size);
    void write_rsp_reg(uint32_t& address, uint64_t* src, DataSize size);
    void write_rsp_stat(uint32_t& address, uint64_t* src, DataSize size);
    void write_dp(uint32_t& address, uint64_t* src, DataSize size);
    void write_dps(uint32_t& address, uint64_t* src, DataSize size);
    void write_mi(uint32_t& address, uint64_t* src, DataSize size);
    void write_vi(uint32_t& address, uint64_t* src, DataSize size);
    void write_ai(uint32_t& address, uint64_t* src, DataSize size);
    void write_pi(uint32_t& address, uint64_t* src, DataSize size);
    void write_ri(uint32_t& address, uint64_t* src, DataSize size);
    void write_si(uint32_t& address, uint64_t* src, DataSize size);
    void write_flashram_dummy(uint32_t& address, uint64_t* src, DataSize size);
    void write_flashram_command(uint32_t& address, uint64_t* src, DataSize size);
    void write_rom(uint32_t& address, uint64_t* src, DataSize size);
    void write_pif(uint32_t& address, uint64_t* src, DataSize size);
    void write_rdramFB(uint32_t& address, uint64_t* src, DataSize size);

    // helpers
    void update_MI_init_mode_reg(void);
    void update_MI_intr_mask_reg(void);
    void update_sp_reg(void);
    void prepare_rsp(void);

private:

    memptr readmem_table[0x10000];
    memptr writemem_table[0x10000];

    uint32_t* readvi_table[0x10000];
    uint32_t* readmi_table[0x10000];
    uint32_t* readrdram_table[0x10000];
    uint32_t* readsp_table[0x10000];
    uint32_t* readsp_stat_table[0x10000];
    uint32_t* readai_table[0x10000];
    uint32_t* readpi_table[0x10000];
    uint32_t* readri_table[0x10000];
    uint32_t* readsi_table[0x10000];
    uint32_t* readdp_table[0x10000];
    uint32_t* readdps_table[0x10000];

    uint32_t trash;
    uint32_t _rom_lastwrite = 0;
};
