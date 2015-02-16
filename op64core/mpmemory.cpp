#include <algorithm>

#include "logger.h"
#include "util.h"

#include "mpmemory.h"
#include "rom.h"
#include "icpu.h"
#include "interrupthandler.h"
#include "cp0.h"
#include "dma.h"
#include "pif.h"
#include "flashram.h"
#include "plugins.h"
#include "gfxplugin.h"
#include "audioplugin.h"
#include "rspplugin.h"
#include "mpmemory_regs.h"
#include "ai_controller.h"


#define MEM_NOT_IMPLEMENTED() \
    Bus::stop = true; \
    LOG_ERROR("Memory: Function %s in %s line %i not implemented. Stopping...", __func__, __FILE__, __LINE__); \
    LOG_VERBOSE("Address: %X", address);


#define BSHIFT(a) (((a & 3) ^ 3) << 3)
#define HSHIFT(a) (((a & 2) ^ 2) << 3)

static FrameBufferInfo fbInfo[6];
static char framebufferRead[0x800];


static inline void masked_write(uint32_t* dst, uint32_t value, uint32_t mask)
{
    *dst = (*dst & ~mask) | (value & mask);
}

void MPMemory::initialize(void)
{
    LOG_INFO("Memory: initializing...");
    IMemory::initialize();

    fill_array(readmem_table, 0, 0x10000, &MPMemory::read_nomem);
    fill_array(writemem_table, 0, 0x10000, &MPMemory::write_nomem);

    // init rdram
    fill_array(_rdram, 0, 0x800000 / 4, 0);

    fill_array(readmem_table, 0x8000, 0x80, &MPMemory::read_rdram);
    fill_array(readmem_table, 0xa000, 0x80, &MPMemory::read_rdram);
    fill_array(writemem_table, 0x8000, 0x80, &MPMemory::write_rdram);
    fill_array(writemem_table, 0xa000, 0x80, &MPMemory::write_rdram);

    fill_array(readmem_table, 0x8080, 0x370, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa080, 0x370, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8080, 0x370, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa080, 0x370, &MPMemory::write_nothing);

    // rdram reg
    readmem_table[0x83f0] = &MPMemory::read_rdram_reg;
    readmem_table[0xa3f0] = &MPMemory::read_rdram_reg;
    writemem_table[0x83f0] = &MPMemory::write_rdram_reg;
    writemem_table[0xa3f0] = &MPMemory::write_rdram_reg;
    fill_array(_rdram_reg, 0, RDRAM_NUM_REGS, 0);

    fill_array(readmem_table, 0x83f1, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa3f1, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x83f1, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa3f1, 0xf, &MPMemory::write_nothing);


    // sp mem
    readmem_table[0x8400] = &MPMemory::read_rsp_mem;
    readmem_table[0xa400] = &MPMemory::read_rsp_mem;
    writemem_table[0x8400] = &MPMemory::write_rsp_mem;
    writemem_table[0xa400] = &MPMemory::write_rsp_mem;

    fill_array(_SP_DMEM, 0, (0x1000 / 4), 0);
    fill_array(_SP_IMEM, 0, (0x1000 / 4), 0);

    fill_array(readmem_table, 0x8401, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa401, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8401, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa401, 0x3, &MPMemory::write_nothing);

    //sp reg
    readmem_table[0x8404] = &MPMemory::read_rsp_reg;
    readmem_table[0xa404] = &MPMemory::read_rsp_reg;
    writemem_table[0x8404] = &MPMemory::write_rsp_reg;
    writemem_table[0xa404] = &MPMemory::write_rsp_reg;
    fill_array(_sp_reg, 0, SP_NUM_REGS, 0);
    _sp_reg[SP_STATUS_REG] = 1;

    fill_array(readmem_table, 0x8405, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa405, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8405, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa405, 0x3, &MPMemory::write_nothing);

    readmem_table[0x8408] = &MPMemory::read_rsp_stat;
    readmem_table[0xa408] = &MPMemory::read_rsp_stat;
    writemem_table[0x8408] = &MPMemory::write_rsp_stat;
    writemem_table[0xa408] = &MPMemory::write_rsp_stat;
    fill_array(_sp2_reg, 0, SP2_NUM_REGS, 0);

    fill_array(readmem_table, 0x8409, 0x7, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa409, 0x7, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8409, 0x7, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa409, 0x7, &MPMemory::write_nothing);

    // dp reg
    readmem_table[0x8410] = &MPMemory::read_dp;
    readmem_table[0xa410] = &MPMemory::read_dp;
    writemem_table[0x8410] = &MPMemory::write_dp;
    writemem_table[0xa410] = &MPMemory::write_dp;
    fill_array(_dp_reg, 0, DPC_NUM_REGS, 0);

    fill_array(readmem_table, 0x8411, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa411, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8411, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa411, 0xf, &MPMemory::write_nothing);

    // dps reg
    readmem_table[0x8420] = &MPMemory::read_dps;
    readmem_table[0xa420] = &MPMemory::read_dps;
    writemem_table[0x8420] = &MPMemory::write_dps;
    writemem_table[0xa420] = &MPMemory::write_dps;
    fill_array(_dps_reg, 0, DPS_NUM_REGS, 0);

    fill_array(readmem_table, 0x8421, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa421, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8421, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa421, 0xf, &MPMemory::write_nothing);


    // mi reg
    readmem_table[0xa830] = &MPMemory::read_mi;
    readmem_table[0xa430] = &MPMemory::read_mi;
    writemem_table[0xa830] = &MPMemory::write_mi;
    writemem_table[0xa430] = &MPMemory::write_mi;
    fill_array(_mi_reg, 0, MI_NUM_REGS, 0);
    _mi_reg[MI_VERSION_REG] = 0x02020102;

    fill_array(readmem_table, 0x8431, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa431, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8431, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa431, 0xf, &MPMemory::write_nothing);

    // vi reg
    readmem_table[0x8440] = &MPMemory::read_vi;
    readmem_table[0xa440] = &MPMemory::read_vi;
    writemem_table[0x8440] = &MPMemory::write_vi;
    writemem_table[0xa440] = &MPMemory::write_vi;
    fill_array(_vi_reg, 0, VI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8441, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa441, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8441, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa441, 0xf, &MPMemory::write_nothing);


    // ai reg
    readmem_table[0x8450] = &MPMemory::read_ai;
    readmem_table[0xa450] = &MPMemory::read_ai;
    writemem_table[0x8450] = &MPMemory::write_ai;
    writemem_table[0xa450] = &MPMemory::write_ai;
    fill_array(&Bus::ai.reg[0], 0, AI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8451, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa451, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8451, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa451, 0xf, &MPMemory::write_nothing);

    // pi reg
    readmem_table[0x8460] = &MPMemory::read_pi;
    readmem_table[0xa460] = &MPMemory::read_pi;
    writemem_table[0x8460] = &MPMemory::write_pi;
    writemem_table[0xa460] = &MPMemory::write_pi;
    fill_array(_pi_reg, 0, PI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8461, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa461, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8461, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa461, 0xf, &MPMemory::write_nothing);

    // ri reg
    readmem_table[0x8470] = &MPMemory::read_ri;
    readmem_table[0xa470] = &MPMemory::read_ri;
    writemem_table[0x8470] = &MPMemory::write_ri;
    writemem_table[0xa470] = &MPMemory::write_ri;
    fill_array(_ri_reg, 0, RI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8471, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa471, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8471, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa471, 0xf, &MPMemory::write_nothing);

    // si reg
    readmem_table[0x8480] = &MPMemory::read_si;
    readmem_table[0xa480] = &MPMemory::read_si;
    writemem_table[0x8480] = &MPMemory::write_si;
    writemem_table[0xa480] = &MPMemory::write_si;
    fill_array(_si_reg, 0, SI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8481, 0x37f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa481, 0x37f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8481, 0x37f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa481, 0x37f, &MPMemory::write_nothing);

    // flashram
    readmem_table[0x8800] = &MPMemory::read_flashram_status;
    readmem_table[0xa800] = &MPMemory::read_flashram_status;
    readmem_table[0x8801] = &MPMemory::read_nothing;
    readmem_table[0xa801] = &MPMemory::read_nothing;
    writemem_table[0x8800] = &MPMemory::write_flashram_dummy;
    writemem_table[0xa800] = &MPMemory::write_flashram_dummy;
    writemem_table[0x8801] = &MPMemory::write_flashram_command;
    writemem_table[0xa801] = &MPMemory::write_flashram_command;

    fill_array(readmem_table, 0x8802, 0x7fe, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa802, 0x7fe, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8802, 0x7fe, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa802, 0x7fe, &MPMemory::write_nothing);

    // rom
    uint32_t rom_size = Bus::rom->getSize();
    fill_array(readmem_table, 0x9000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(readmem_table, 0xb000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(writemem_table, 0x9000, (rom_size >> 16), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000, (rom_size >> 16), &MPMemory::write_rom);

    fill_array(readmem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(readmem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::write_nothing);


    // pif
    readmem_table[0x9fc0] = &MPMemory::read_pif;
    readmem_table[0xbfc0] = &MPMemory::read_pif;
    writemem_table[0x9fc0] = &MPMemory::write_pif;
    writemem_table[0xbfc0] = &MPMemory::write_pif;
    fill_array(_PIF_RAM, 0, (0x40 / 4), 0);

    fill_array(readmem_table, 0x9fc1, 0x3f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xbfc1, 0x3f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9fc1, 0x3f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xbfc1, 0x3f, &MPMemory::write_nothing);

    // r/w size table
    readsize[SIZE_BYTE] = &MPMemory::read_size_byte;
    readsize[SIZE_HWORD] = &MPMemory::read_size_half;
    readsize[SIZE_WORD] = &MPMemory::read_size_word;
    readsize[SIZE_DWORD] = &MPMemory::read_size_dword;

    writesize[SIZE_BYTE] = &MPMemory::write_size_byte;
    writesize[SIZE_HWORD] = &MPMemory::write_size_half;
    writesize[SIZE_WORD] = &MPMemory::write_size_word;
    writesize[SIZE_DWORD] = &MPMemory::write_size_dword;

    fbInfo[0].addr = 0;

    Bus::ai.fifo[0].delay = 0;
    Bus::ai.fifo[1].delay = 0;

    Bus::ai.fifo[0].length = 0;
    Bus::ai.fifo[1].length = 0;
}

//////////////////////////////////////////////////////////////////////////
// sized r/w

void MPMemory::read_size_byte(readfn read_func, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = BSHIFT(address);
    (this->*read_func)(address, &data);
    *dest = (data >> shift) & 0xff;
}

void MPMemory::read_size_half(readfn read_func, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = HSHIFT(address);
    (this->*read_func)(address, &data);
    *dest = (data >> shift) & 0xffff;
}

void MPMemory::read_size_word(readfn read_func, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    (this->*read_func)(address, &data);
    *dest = data;
}

void MPMemory::read_size_dword(readfn read_func, uint32_t& address, uint64_t* dest)
{
    uint32_t data[2];
    (this->*read_func)(address, &data[0]);
    (this->*read_func)(address + 4, &data[1]);
    *dest = ((uint64_t)data[0] << 32) | data[1];
}

void MPMemory::write_size_byte(writefn write_func, uint32_t address, uint64_t src)
{
    uint32_t shift = BSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xff << shift;

    (this->*write_func)(address, data, mask);
}

void MPMemory::write_size_half(writefn write_func, uint32_t address, uint64_t src)
{
    uint32_t shift = HSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xffff << shift;

    (this->*write_func)(address, data, mask);
}

void MPMemory::write_size_word(writefn write_func, uint32_t address, uint64_t src)
{
    (this->*write_func)(address, src, ~0U);
}

void MPMemory::write_size_dword(writefn write_func, uint32_t address, uint64_t src)
{
    (this->*write_func)(address, src >> 32, ~0U);
    (this->*write_func)(address + 4, src, ~0U);
}

//////////////////////////////////////////////////////////////////////////
// nothing

void MPMemory::read_nothing(uint32_t& address, uint64_t* dest, DataSize size)
{
    *dest = 0;
}

void MPMemory::write_nothing(uint32_t address, uint64_t src, DataSize size)
{
    // does nothing
}

//////////////////////////////////////////////////////////////////////////
// nomem

void MPMemory::read_nomem(uint32_t& address, uint64_t* dest, DataSize size)
{
    address = TLB::virtual_to_physical_address(address, TLB_READ);
    if (address == 0x00000000)
        return;

    readmem(address, dest, size);
}

void MPMemory::write_nomem(uint32_t address, uint64_t src, DataSize size)
{
    address = TLB::virtual_to_physical_address(address, TLB_WRITE);
    if (address == 0x00000000) return;

    writemem(address, src, size);
}

//////////////////////////////////////////////////////////////////////////
// rdram

void MPMemory::read_rdram_func(uint32_t address, uint32_t* dest)
{
    *dest = _rdram[RDRAM_ADDRESS(address)];
}

void MPMemory::read_rdram(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rdram_func, address, dest);
}

void MPMemory::read_rdramFB_func(uint32_t address, uint32_t* dest)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        if (fbInfo[i].addr)
        {
            uint32_t start = fbInfo[i].addr & 0x7FFFFF;
            uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
            if ((address & 0x7FFFFF) >= start && (address & 0x7FFFFF) <= end &&
                framebufferRead[(address & 0x7FFFFF) >> 12])
            {
                Bus::plugins->gfx()->fbRead(address);
                framebufferRead[(address & 0x7FFFFF) >> 12] = 0;
            }
        }
    }

    read_rdram_func(address, dest);
}

void MPMemory::read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rdramFB_func, address, dest);
}

void MPMemory::write_rdram_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = RDRAM_ADDRESS(address);

    masked_write(&_rdram[addr], data, mask);
}

void MPMemory::write_rdram(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rdram_func, address, src);
}

void MPMemory::write_rdramFB_func(uint32_t address, uint32_t data, uint32_t mask)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        if (fbInfo[i].addr)
        {
            uint32_t start = fbInfo[i].addr & 0x7FFFFF;
            uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
            if ((address & 0x7FFFFF) >= start && (address & 0x7FFFFF) <= end)
            {
                Bus::plugins->gfx()->fbWrite(address, 4);
            }
        }
    }

    write_rdram_func(address, data, mask);
}

void MPMemory::write_rdramFB(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rdramFB_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rdram reg

void MPMemory::read_rdram_reg_func(uint32_t address, uint32_t* dest)
{
    *dest = _rdram_reg[RDRAM_REG(address)];
}

void MPMemory::read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rdram_reg_func, address, dest);
}

void MPMemory::write_rdram_reg_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = RDRAM_REG(address);

    masked_write(&_rdram_reg[reg], data, mask);
}

void MPMemory::write_rdram_reg(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rdram_reg_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp mem

void MPMemory::read_rsp_mem_func(uint32_t address, uint32_t* dest)
{
    *dest = _SP_MEM[RSP_ADDRESS(address)];
}

void MPMemory::read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rsp_mem_func, address, dest);
}

void MPMemory::write_rsp_mem_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = RSP_ADDRESS(address);

    masked_write(&_SP_MEM[addr], data, mask);
}

void MPMemory::write_rsp_mem(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rsp_mem_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp reg

void MPMemory::read_rsp_reg_func(uint32_t address, uint32_t* dest)
{
    uint32_t reg = RSP_REG(address);

    *dest = _sp_reg[reg];

    if (reg == SP_SEMAPHORE_REG)
    {
        _sp_reg[SP_SEMAPHORE_REG] = 1;
    }
}

void MPMemory::read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rsp_reg_func, address, dest);
}

void MPMemory::write_rsp_reg_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = RSP_REG(address);

    switch (reg)
    {
    case SP_STATUS_REG:
        update_sp_reg(data & mask);
    case SP_DMA_FULL_REG:
    case SP_DMA_BUSY_REG:
        return;
    }

    masked_write(&_sp_reg[reg], data, mask);

    switch (reg)
    {
    case SP_RD_LEN_REG:
        DMA::writeSP();
        break;
    case SP_WR_LEN_REG:
        DMA::readSP();
        break;
    case SP_SEMAPHORE_REG:
        _sp_reg[SP_SEMAPHORE_REG] = 0;
        break;
    }
}

void MPMemory::write_rsp_reg(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rsp_reg_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp stat

void MPMemory::read_rsp_stat_func(uint32_t address, uint32_t* dest)
{
    uint32_t reg = RSP_REG2(address);

    *dest = _sp2_reg[reg];
}

void MPMemory::read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rsp_stat_func, address, dest);
}

void MPMemory::write_rsp_stat_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = RSP_REG2(address);

    masked_write(&_sp2_reg[reg], data, mask);
}

void MPMemory::write_rsp_stat(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rsp_stat_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dpc

void MPMemory::read_dp_func(uint32_t address, uint32_t* dest)
{
    *dest = _dp_reg[DPC_REG(address)];
}

void MPMemory::read_dp(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_dp_func, address, dest);
}

void MPMemory::write_dp_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = DPC_REG(address);

    switch (reg)
    {
    case DPC_STATUS_REG:
        if (updateDPC(data & mask))
            prepare_rsp();
    case DPC_CURRENT_REG:
    case DPC_CLOCK_REG:
    case DPC_BUFBUSY_REG:
    case DPC_PIPEBUSY_REG:
    case DPC_TMEM_REG:
        return;
    }

    masked_write(&_dp_reg[reg], data, mask);

    switch (reg)
    {
    case DPC_START_REG:
        _dp_reg[DPC_CURRENT_REG] = _dp_reg[DPC_START_REG];
        break;
    case DPC_END_REG:
        Bus::plugins->gfx()->ProcessRDPList();
        _mi_reg[MI_INTR_REG] |= 0x20;
        Bus::interrupt->checkInterrupt();
        break;
    }
}

void MPMemory::write_dp(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_dp_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dps

void MPMemory::read_dps_func(uint32_t address, uint32_t* dest)
{
    *dest = _dps_reg[DPS_REG(address)];
}

void MPMemory::read_dps(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_dps_func, address, dest);
}

void MPMemory::write_dps_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = DPS_REG(address);

    masked_write(&_dps_reg[reg], data, mask);
}

void MPMemory::write_dps(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_dps_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// mi

void MPMemory::read_mi_func(uint32_t address, uint32_t* dest)
{
    *dest = _mi_reg[MI_REG(address)];
}

void MPMemory::read_mi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_mi_func, address, dest);
}

void MPMemory::write_mi_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = MI_REG(address);

    switch (reg)
    {
    case MI_INIT_MODE_REG:
        if (update_mi_init_mode(data & mask))
        {
            /* clear DP interrupt */
            _mi_reg[MI_INTR_REG] &= ~0x20;
            Bus::interrupt->checkInterrupt();
        }
        break;
    case MI_INTR_MASK_REG:
        update_mi_intr_mask(data & mask);

        Bus::interrupt->checkInterrupt();
        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (Bus::next_interrupt <= Bus::cp0_reg[CP0_COUNT_REG])
            Bus::interrupt->generateInterrupt();

        break;
    }
}

void MPMemory::write_mi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_mi_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// vi

void MPMemory::read_vi_func(uint32_t address, uint32_t* dest)
{
    uint32_t reg = VI_REG(address);

    if (reg == VI_CURRENT_REG)
    {
        Bus::cpu->getCp0()->updateCount(*Bus::PC);
        if (_vi_reg[VI_V_SYNC_REG])
        {
            _vi_reg[VI_CURRENT_REG] = (Bus::vi_delay - (Bus::next_vi - Bus::cp0_reg[CP0_COUNT_REG])) / (Bus::vi_delay / _vi_reg[VI_V_SYNC_REG]);
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[VI_CURRENT_REG] & (~1)) | Bus::vi_field;
        }
        else
        {
            _vi_reg[VI_CURRENT_REG] = 0;
        }
    }

    *dest = _vi_reg[reg];
}

void MPMemory::read_vi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_vi_func, address, dest);
}

void MPMemory::write_vi_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = VI_REG(address);

    switch (reg)
    {
    case VI_STATUS_REG:
        if ((_vi_reg[VI_STATUS_REG] & mask) != (data & mask))
        {
            masked_write(&_vi_reg[VI_STATUS_REG], data, mask);
            if (Bus::plugins->gfx()->ViStatusChanged != nullptr)
            {
                Bus::plugins->gfx()->ViStatusChanged();
            }
        }
        return;

    case VI_WIDTH_REG:
        if ((_vi_reg[VI_WIDTH_REG] & mask) != (data & mask))
        {
            masked_write(&_vi_reg[VI_WIDTH_REG], data, mask);
            if (Bus::plugins->gfx()->ViWidthChanged != nullptr)
            {
                Bus::plugins->gfx()->ViWidthChanged();
            }
        }
        return;

    case VI_CURRENT_REG:
        _mi_reg[MI_INTR_REG] &= ~0x8;
        Bus::interrupt->checkInterrupt();
        return;
    }

    masked_write(&_vi_reg[reg], data, mask);
}

void MPMemory::write_vi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_vi_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ai

void MPMemory::read_ai_func(uint32_t address, uint32_t* dest)
{
    uint32_t reg = AI_REG(address);

    if (reg == AI_LEN_REG)
    {
        Bus::cpu->getCp0()->updateCount(*Bus::PC);
        if (Bus::ai.fifo[0].delay != 0 && Bus::interrupt->findEvent(AI_INT) != 0 && (Bus::interrupt->findEvent(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG]) < 0x80000000)
        {
            *dest = ((Bus::interrupt->findEvent(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG])*(int64_t)Bus::ai.fifo[0].length) / Bus::ai.fifo[0].delay;
        }
        else
        {
            *dest = 0;
        }
    }
    else
    {
        *dest = Bus::ai.reg[reg];
    }
}

void MPMemory::read_ai(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_ai_func, address, dest);
}

void MPMemory::write_ai_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = AI_REG(address);

    unsigned int freq, delay = 0;
    switch (reg)
    {
    case AI_LEN_REG:
        masked_write(&Bus::ai.reg[AI_LEN_REG], data, mask);
        if (Bus::plugins->audio()->LenChanged != nullptr)
        {
            Bus::plugins->audio()->LenChanged();
        }

        freq = Bus::rom->getAiDACRate() / (Bus::ai.reg[AI_DACRATE_REG] + 1);
        if (freq)
            delay = (unsigned int)(((uint64_t)Bus::ai.reg[AI_LEN_REG] * Bus::vi_delay * Bus::rom->getViLimit()) / (freq * 4));

        if (Bus::ai.reg[AI_STATUS_REG] & 0x40000000) // busy
        {
            Bus::ai.fifo[1].delay = delay;
            Bus::ai.fifo[1].length = Bus::ai.reg[AI_LEN_REG];
            Bus::ai.reg[AI_STATUS_REG] |= 0x80000000;
        }
        else
        {
            Bus::ai.fifo[0].delay = delay;
            Bus::ai.fifo[0].length = Bus::ai.reg[AI_LEN_REG];
            Bus::cpu->getCp0()->updateCount(*Bus::PC);
            Bus::interrupt->addInterruptEvent(AI_INT, delay);
            Bus::ai.reg[AI_STATUS_REG] |= 0x40000000;
        }
        return;

    case AI_STATUS_REG:
        _mi_reg[MI_INTR_REG] &= ~0x4;
        Bus::interrupt->checkInterrupt();
        return;

    case AI_DACRATE_REG:
        if ((Bus::ai.reg[AI_DACRATE_REG] & mask) != (data & mask))
        {
            masked_write(&Bus::ai.reg[AI_DACRATE_REG], data, mask);
            Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
        }
        return;
    }

    masked_write(&Bus::ai.reg[reg], data, mask);
}

void MPMemory::write_ai(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_ai_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pi

void MPMemory::read_pi_func(uint32_t address, uint32_t* dest)
{
    *dest = _pi_reg[PI_REG(address)];
}

void MPMemory::read_pi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_pi_func, address, dest);
}

void MPMemory::write_pi_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = PI_REG(address);

    switch (reg)
    {
    case PI_RD_LEN_REG:
        masked_write(&_pi_reg[PI_RD_LEN_REG], data, mask);
        DMA::readPI();
        return;

    case PI_WR_LEN_REG:
        masked_write(&_pi_reg[PI_WR_LEN_REG], data, mask);
        DMA::writePI();
        return;

    case PI_STATUS_REG:
        if (data & mask & 2)
        {
            _mi_reg[MI_INTR_REG] &= ~0x10;
            Bus::interrupt->checkInterrupt();
        }

        return;

    case PI_BSD_DOM1_LAT_REG:
    case PI_BSD_DOM1_PWD_REG:
    case PI_BSD_DOM1_PGS_REG:
    case PI_BSD_DOM1_RLS_REG:
    case PI_BSD_DOM2_LAT_REG:
    case PI_BSD_DOM2_PWD_REG:
    case PI_BSD_DOM2_PGS_REG:
    case PI_BSD_DOM2_RLS_REG:
        masked_write(&_pi_reg[reg], data & 0xff, mask);
        return;
    }

    masked_write(&_pi_reg[reg], data, mask);
}

void MPMemory::write_pi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_pi_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ri

void MPMemory::read_ri_func(uint32_t address, uint32_t* dest)
{
    *dest = _ri_reg[RI_REG(address)];
}

void MPMemory::read_ri(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_ri_func, address, dest);
}

void MPMemory::write_ri_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = RI_REG(address);

    masked_write(&_ri_reg[reg], data, mask);
}

void MPMemory::write_ri(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_ri_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// si

void MPMemory::read_si_func(uint32_t address, uint32_t* dest)
{
    *dest = _si_reg[SI_REG(address)];
}

void MPMemory::read_si(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_si_func, address, dest);
}

void MPMemory::write_si_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = SI_REG(address);

    switch (reg)
    {
    case SI_DRAM_ADDR_REG:
        masked_write(&_si_reg[SI_DRAM_ADDR_REG], data, mask);
        break;

    case SI_PIF_ADDR_RD64B_REG:
        masked_write(&_si_reg[SI_PIF_ADDR_RD64B_REG], data, mask);
        DMA::readSI();
        break;

    case SI_PIF_ADDR_WR64B_REG:
        masked_write(&_si_reg[SI_PIF_ADDR_WR64B_REG], data, mask);
        DMA::writeSI();
        break;

    case SI_STATUS_REG:
        _si_reg[SI_STATUS_REG] &= ~0x1000;
        _mi_reg[MI_INTR_REG] &= ~0x2;
        Bus::interrupt->checkInterrupt();
        break;
    }
}

void MPMemory::write_si(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_si_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// flashram

void MPMemory::read_flashram_status_func(uint32_t address, uint32_t* dest)
{
    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        *dest = Bus::flashram->readFlashStatus();
    }
    else
    {
        LOG_WARNING("Reading flashram command with non-flashram save type");
    }
}

void MPMemory::read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_flashram_status_func, address, dest);
}

void MPMemory::write_flashram_dummy(uint32_t address, uint64_t src, DataSize size)
{
}

void MPMemory::write_flashram_command_func(uint32_t address, uint32_t data, uint32_t mask)
{
    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        Bus::flashram->writeFlashCommand(data & mask);
    }
    else
    {
        LOG_WARNING("Writing flashram command with non-flashram save type");
    }
}

void MPMemory::write_flashram_command(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_flashram_command_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rom

void MPMemory::read_rom_func(uint32_t address, uint32_t* dest)
{
    uint32_t addr = ROM_ADDRESS(address);

    if (_rom_lastwrite != 0)
    {
        *dest = _rom_lastwrite;
        _rom_lastwrite = 0;
    }
    else
    {
        *dest = *(uint32_t*)(Bus::rom_image + addr);
    }
}

void MPMemory::read_rom(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_rom_func, address, dest);
}

void MPMemory::write_rom_func(uint32_t address, uint32_t data, uint32_t mask)
{
    _rom_lastwrite = data & mask;
}

void MPMemory::write_rom(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_rom_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pif

void MPMemory::read_pif_func(uint32_t address, uint32_t* dest)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR("Reading a byte in PIF at invalid address 0x%x", address);
        *dest = 0;
        return;
    }

    *dest = byteswap_u32(*(uint32_t*)(Bus::pif_ram8 + addr));
}

void MPMemory::read_pif(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(&MPMemory::read_pif_func, address, dest);
}

void MPMemory::write_pif_func(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR("Invalid PIF address: %08x", address);
        return;
    }

    masked_write((uint32_t*)(&Bus::pif_ram8[addr]), byteswap_u32(data), byteswap_u32(mask));

    if ((addr == 0x3c) && (mask & 0xff))
    {
        if (Bus::pif_ram8[0x3f] == 0x08)
        {
            Bus::pif_ram8[0x3f] = 0;
            Bus::cpu->getCp0()->updateCount(*Bus::PC);
            Bus::interrupt->addInterruptEvent(SI_INT, 0x900);
        }
        else
        {
            Bus::pif->pifWrite();
        }
    }
}

void MPMemory::write_pif(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(&MPMemory::write_pif_func, address, src);
}

//////////////////////////////////////////////////////////////////////////
// helpers

bool MPMemory::update_mi_init_mode(uint32_t w)
{
    bool clear_dp = false;

    _mi_reg[MI_INIT_MODE_REG] &= ~0x7F; // init_length
    _mi_reg[MI_INIT_MODE_REG] |= w & 0x7F;

    if (w & 0x80) // clear init_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x80;
    if (w & 0x100) // set init_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x80;

    if (w & 0x200) // clear ebus_test_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x100;
    if (w & 0x400) // set ebus_test_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x100;

    if (w & 0x800) // clear DP interrupt
    {
        clear_dp = true;
    }

    if (w & 0x1000) // clear RDRAM_reg_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x200;
    if (w & 0x2000) // set RDRAM_reg_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x200;

    return clear_dp;
}


void MPMemory::update_mi_intr_mask(uint32_t w)
{
    if (w & 0x1)   _mi_reg[MI_INTR_MASK_REG] &= ~0x1; // clear SP mask
    if (w & 0x2)   _mi_reg[MI_INTR_MASK_REG] |= 0x1; // set SP mask
    if (w & 0x4)   _mi_reg[MI_INTR_MASK_REG] &= ~0x2; // clear SI mask
    if (w & 0x8)   _mi_reg[MI_INTR_MASK_REG] |= 0x2; // set SI mask
    if (w & 0x10)  _mi_reg[MI_INTR_MASK_REG] &= ~0x4; // clear AI mask
    if (w & 0x20)  _mi_reg[MI_INTR_MASK_REG] |= 0x4; // set AI mask
    if (w & 0x40)  _mi_reg[MI_INTR_MASK_REG] &= ~0x8; // clear VI mask
    if (w & 0x80)  _mi_reg[MI_INTR_MASK_REG] |= 0x8; // set VI mask
    if (w & 0x100) _mi_reg[MI_INTR_MASK_REG] &= ~0x10; // clear PI mask
    if (w & 0x200) _mi_reg[MI_INTR_MASK_REG] |= 0x10; // set PI mask
    if (w & 0x400) _mi_reg[MI_INTR_MASK_REG] &= ~0x20; // clear DP mask
    if (w & 0x800) _mi_reg[MI_INTR_MASK_REG] |= 0x20; // set DP mask
}


void MPMemory::update_sp_reg(uint32_t w)
{
    if (w & 0x1) // clear halt
        _sp_reg[SP_STATUS_REG] &= ~0x1;
    if (w & 0x2) // set halt
        _sp_reg[SP_STATUS_REG] |= 0x1;

    if (w & 0x4) // clear broke
        _sp_reg[SP_STATUS_REG] &= ~0x2;

    if (w & 0x8) // clear SP interrupt
    {
        _mi_reg[MI_INTR_REG] &= ~1;
        Bus::interrupt->checkInterrupt();
    }

    if (w & 0x10) // set SP interrupt
    {
        _mi_reg[MI_INTR_REG] |= 1;
        Bus::interrupt->checkInterrupt();
    }

    if (w & 0x20) // clear single step
        _sp_reg[SP_STATUS_REG] &= ~0x20;
    if (w & 0x40) // set single step
        _sp_reg[SP_STATUS_REG] |= 0x20;

    if (w & 0x80) // clear interrupt on break
        _sp_reg[SP_STATUS_REG] &= ~0x40;
    if (w & 0x100) // set interrupt on break
        _sp_reg[SP_STATUS_REG] |= 0x40;

    if (w & 0x200) // clear signal 0
        _sp_reg[SP_STATUS_REG] &= ~0x80;
    if (w & 0x400) // set signal 0
        _sp_reg[SP_STATUS_REG] |= 0x80;

    if (w & 0x800) // clear signal 1
        _sp_reg[SP_STATUS_REG] &= ~0x100;
    if (w & 0x1000) // set signal 1
        _sp_reg[SP_STATUS_REG] |= 0x100;

    if (w & 0x2000) // clear signal 2
        _sp_reg[SP_STATUS_REG] &= ~0x200;
    if (w & 0x4000) // set signal 2
        _sp_reg[SP_STATUS_REG] |= 0x200;

    if (w & 0x8000) // clear signal 3
        _sp_reg[SP_STATUS_REG] &= ~0x400;
    if (w & 0x10000) // set signal 3
        _sp_reg[SP_STATUS_REG] |= 0x400;

    if (w & 0x20000) // clear signal 4
        _sp_reg[SP_STATUS_REG] &= ~0x800;
    if (w & 0x40000) // set signal 4
        _sp_reg[SP_STATUS_REG] |= 0x800;

    if (w & 0x80000) // clear signal 5
        _sp_reg[SP_STATUS_REG] &= ~0x1000;
    if (w & 0x100000) // set signal 5
        _sp_reg[SP_STATUS_REG] |= 0x1000;

    if (w & 0x200000) // clear signal 6
        _sp_reg[SP_STATUS_REG] &= ~0x2000;
    if (w & 0x400000) // set signal 6
        _sp_reg[SP_STATUS_REG] |= 0x2000;

    if (w & 0x800000) // clear signal 7
        _sp_reg[SP_STATUS_REG] &= ~0x4000;
    if (w & 0x1000000) // set signal 7
        _sp_reg[SP_STATUS_REG] |= 0x4000;

    //if (get_event(SP_INT)) return;
    if (!(w & 0x1) && !(w & 0x4))
        return;

    if (!(_sp_reg[SP_STATUS_REG] & 0x3)) // !halt && !broke
    {
        prepare_rsp();
    }
}



void MPMemory::prepare_rsp(void)
{
    int32_t save_pc = _sp2_reg[SP_PC_REG] & ~0xFFF;

    // Video task
    if (_SP_DMEM[0xFC0 / 4] == 1)
    {
        if (_dp_reg[DPC_STATUS_REG] & 0x2) // DP frozen (DK64, BC)
        {
            // don't do the task now
            // the task will be done when DP is unfreezed (see update_DPC)
            return;
        }

        // unprotecting old frame buffers
        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite &&
            fbInfo[0].addr)
        {
            vec_for (uint32_t i = 0; i < 6; i++)
            {
                if (fbInfo[i].addr)
                {
                    uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                    uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                    start = start >> 16;
                    end = end >> 16;

                    fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdram);
                    fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdram);
                    fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdram);
                    fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdram);
                }
            }
        }

        _sp2_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xffffffff);
        _sp2_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 1000);
        }

        if (_mi_reg[MI_INTR_REG] & 0x20)
        {
            Bus::interrupt->addInterruptEvent(DP_INT, 1000);
        }

        _mi_reg[MI_INTR_REG] &= ~0x21;
        _sp_reg[SP_STATUS_REG] &= ~0x303;

        // protecting new frame buffers
        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite)
        {
            Bus::plugins->gfx()->fbGetInfo(fbInfo);
        }

        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite
            && fbInfo[0].addr)
        {
            vec_for (uint32_t i = 0; i < 6; i++)
            {
                if (fbInfo[i].addr)
                {
                    uint32_t j;
                    uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                    uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                    uint32_t start1 = start;
                    uint32_t end1 = end;

                    start >>= 16;
                    end >>= 16;

                    fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdramFB);
                    fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdramFB);
                    fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdramFB);
                    fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdramFB);

                    start <<= 4;
                    end <<= 4;
                    for (j = start; j <= end; j++)
                    {
                        if (j >= start1 && j <= end1)
                        {
                            framebufferRead[j] = 1;
                        }
                        else
                        {
                            framebufferRead[j] = 0;
                        }
                    }
                }
            }
        }
    }
    // Audio task
    else if (_SP_DMEM[0xFC0 / 4] == 2)
    {
        _sp2_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        _sp2_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 4000/*500*/);
        }

        _mi_reg[MI_INTR_REG] &= ~0x1;
        _sp_reg[SP_STATUS_REG] &= ~0x303;

    }
    // Unknown task
    else
    {
        _sp2_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        _sp2_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 0/*100*/);
        }

        _mi_reg[MI_INTR_REG] &= ~0x1;
        _sp_reg[SP_STATUS_REG] &= ~0x203;
    }
}

bool MPMemory::updateDPC(uint32_t w)
{
    bool do_sp_task_on_unfreeze = false;

    if (w & 0x1) // clear xbus_dmem_dma
        _dp_reg[DPC_STATUS_REG] &= ~0x1;

    if (w & 0x2) // set xbus_dmem_dma
        _dp_reg[DPC_STATUS_REG] |= 0x1;

    if (w & 0x4) // clear freeze
    {
        _dp_reg[DPC_STATUS_REG] &= ~0x2;

        // see do_SP_task for more info
        if (!(_sp_reg[SP_STATUS_REG] & 0x3)) // !halt && !broke
        {
            do_sp_task_on_unfreeze = true;
        }
    }

    if (w & 0x8) // set freeze
        _dp_reg[DPC_STATUS_REG] |= 0x2;

    if (w & 0x10) // clear flush
        _dp_reg[DPC_STATUS_REG] &= ~0x4;

    if (w & 0x20) // set flush
        _dp_reg[DPC_STATUS_REG] |= 0x4;

    return do_sp_task_on_unfreeze;
}

