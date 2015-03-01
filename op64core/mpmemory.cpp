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

#include "rcp.h"


#define MEM_NOT_IMPLEMENTED() \
    Bus::stop = true; \
    LOG_ERROR("Memory: Function %s in %s line %i not implemented. Stopping...", __func__, __FILE__, __LINE__); \
    LOG_VERBOSE("Address: %X", address);


static FrameBufferInfo fbInfo[6];
static char framebufferRead[0x800];


void MPMemory::initialize(void)
{
    LOG_INFO("Memory: initializing...");
    IMemory::initialize();

    fill_array(readmem_table, 0, 0x10000, &MPMemory::read_nomem);
    fill_array(writemem_table, 0, 0x10000, &MPMemory::write_nomem);

    // init rdram
    fill_array(Bus::rdram->mem, 0, RDRAM_SIZE / 4, 0);

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
    fill_array(Bus::rdram->reg, 0, RDRAM_NUM_REGS, 0);

    fill_array(readmem_table, 0x83f1, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa3f1, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x83f1, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa3f1, 0xf, &MPMemory::write_nothing);


    // sp mem
    readmem_table[0x8400] = &MPMemory::read_rsp_mem;
    readmem_table[0xa400] = &MPMemory::read_rsp_mem;
    writemem_table[0x8400] = &MPMemory::write_rsp_mem;
    writemem_table[0xa400] = &MPMemory::write_rsp_mem;

    fill_array(Bus::rcp->sp.dmem, 0, (0x1000 / 4), 0);
    fill_array(Bus::rcp->sp.imem, 0, (0x1000 / 4), 0);

    fill_array(readmem_table, 0x8401, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa401, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8401, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa401, 0x3, &MPMemory::write_nothing);

    //sp reg
    readmem_table[0x8404] = &MPMemory::read_rsp_reg;
    readmem_table[0xa404] = &MPMemory::read_rsp_reg;
    writemem_table[0x8404] = &MPMemory::write_rsp_reg;
    writemem_table[0xa404] = &MPMemory::write_rsp_reg;
    fill_array(Bus::rcp->sp.reg, 0, SP_NUM_REGS, 0);
    Bus::rcp->sp.reg[SP_STATUS_REG] = 1;

    fill_array(readmem_table, 0x8405, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa405, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8405, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa405, 0x3, &MPMemory::write_nothing);

    readmem_table[0x8408] = &MPMemory::read_rsp_stat;
    readmem_table[0xa408] = &MPMemory::read_rsp_stat;
    writemem_table[0x8408] = &MPMemory::write_rsp_stat;
    writemem_table[0xa408] = &MPMemory::write_rsp_stat;
    fill_array(Bus::rcp->sp.stat, 0, SP2_NUM_REGS, 0);

    fill_array(readmem_table, 0x8409, 0x7, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa409, 0x7, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8409, 0x7, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa409, 0x7, &MPMemory::write_nothing);

    // dp reg
    readmem_table[0x8410] = &MPMemory::read_dp;
    readmem_table[0xa410] = &MPMemory::read_dp;
    writemem_table[0x8410] = &MPMemory::write_dp;
    writemem_table[0xa410] = &MPMemory::write_dp;
    fill_array(Bus::rcp->dpc.reg, 0, DPC_NUM_REGS, 0);

    fill_array(readmem_table, 0x8411, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa411, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8411, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa411, 0xf, &MPMemory::write_nothing);

    // dps reg
    readmem_table[0x8420] = &MPMemory::read_dps;
    readmem_table[0xa420] = &MPMemory::read_dps;
    writemem_table[0x8420] = &MPMemory::write_dps;
    writemem_table[0xa420] = &MPMemory::write_dps;
    fill_array(Bus::rcp->dps.reg, 0, DPS_NUM_REGS, 0);

    fill_array(readmem_table, 0x8421, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa421, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8421, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa421, 0xf, &MPMemory::write_nothing);


    // mi reg
    readmem_table[0xa830] = &MPMemory::read_mi;
    readmem_table[0xa430] = &MPMemory::read_mi;
    writemem_table[0xa830] = &MPMemory::write_mi;
    writemem_table[0xa430] = &MPMemory::write_mi;
    fill_array(Bus::rcp->mi.reg, 0, MI_NUM_REGS, 0);
    Bus::rcp->mi.reg[MI_VERSION_REG] = 0x02020102;

    fill_array(readmem_table, 0x8431, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa431, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8431, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa431, 0xf, &MPMemory::write_nothing);

    // vi reg
    readmem_table[0x8440] = &MPMemory::read_vi;
    readmem_table[0xa440] = &MPMemory::read_vi;
    writemem_table[0x8440] = &MPMemory::write_vi;
    writemem_table[0xa440] = &MPMemory::write_vi;
    fill_array(Bus::rcp->vi.reg, 0, VI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8441, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa441, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8441, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa441, 0xf, &MPMemory::write_nothing);


    // ai reg
    readmem_table[0x8450] = &MPMemory::read_ai;
    readmem_table[0xa450] = &MPMemory::read_ai;
    writemem_table[0x8450] = &MPMemory::write_ai;
    writemem_table[0xa450] = &MPMemory::write_ai;
    fill_array(Bus::rcp->ai.reg, 0, AI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8451, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa451, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8451, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa451, 0xf, &MPMemory::write_nothing);

    // pi reg
    readmem_table[0x8460] = &MPMemory::read_pi;
    readmem_table[0xa460] = &MPMemory::read_pi;
    writemem_table[0x8460] = &MPMemory::write_pi;
    writemem_table[0xa460] = &MPMemory::write_pi;
    fill_array(Bus::rcp->pi.reg, 0, PI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8461, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa461, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8461, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa461, 0xf, &MPMemory::write_nothing);

    // ri reg
    readmem_table[0x8470] = &MPMemory::read_ri;
    readmem_table[0xa470] = &MPMemory::read_ri;
    writemem_table[0x8470] = &MPMemory::write_ri;
    writemem_table[0xa470] = &MPMemory::write_ri;
    fill_array(Bus::rcp->ri.reg, 0, RI_NUM_REGS, 0);

    fill_array(readmem_table, 0x8471, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa471, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8471, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa471, 0xf, &MPMemory::write_nothing);

    // si reg
    readmem_table[0x8480] = &MPMemory::read_si;
    readmem_table[0xa480] = &MPMemory::read_si;
    writemem_table[0x8480] = &MPMemory::write_si;
    writemem_table[0xa480] = &MPMemory::write_si;
    fill_array(Bus::rcp->si.reg, 0, SI_NUM_REGS, 0);

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
    fill_array(Bus::pif->ram, 0, PIF_RAM_SIZE, 0);

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

    Bus::rcp->ai.fifo[0].delay = 0;
    Bus::rcp->ai.fifo[1].delay = 0;

    Bus::rcp->ai.fifo[0].length = 0;
    Bus::rcp->ai.fifo[1].length = 0;
}

//////////////////////////////////////////////////////////////////////////
// sized r/w

void MPMemory::read_size_byte(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = BSHIFT(address);
    device.read(address, &data);
    *dest = (data >> shift) & 0xff;
}

void MPMemory::read_size_half(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = HSHIFT(address);
    device.read(address, &data);
    *dest = (data >> shift) & 0xffff;
}

void MPMemory::read_size_word(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    device.read(address, &data);
    *dest = data;
}

void MPMemory::read_size_dword(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data[2];
    device.read(address, &data[0]);
    device.read(address + 4, &data[1]);
    *dest = ((uint64_t)data[0] << 32) | data[1];
}

void MPMemory::write_size_byte(RCPInterface& device, uint32_t address, uint64_t src)
{
    uint32_t shift = BSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xff << shift;

    device.write(address, data, mask);
}

void MPMemory::write_size_half(RCPInterface& device, uint32_t address, uint64_t src)
{
    uint32_t shift = HSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xffff << shift;

    device.write(address, data, mask);
}

void MPMemory::write_size_word(RCPInterface& device, uint32_t address, uint64_t src)
{
    device.write(address, src, ~0U);
}

void MPMemory::write_size_dword(RCPInterface& device, uint32_t address, uint64_t src)
{
    device.write(address, src >> 32, ~0U);
    device.write(address + 4, src, ~0U);
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

void MPMemory::read_rdram(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rdram->setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(*Bus::rdram, address, dest);
}

void MPMemory::read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size)
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

    Bus::rdram->setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(*Bus::rdram, address, dest);
}

void MPMemory::write_rdram(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rdram->setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(*Bus::rdram, address, src);
}

void MPMemory::write_rdramFB(uint32_t address, uint64_t src, DataSize size)
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

    Bus::rdram->setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(*Bus::rdram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rdram reg

void MPMemory::read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rdram->setIOMode(RCP_IO_REG);
    (this->*readsize[size])(*Bus::rdram, address, dest);
}

void MPMemory::write_rdram_reg(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rdram->setIOMode(RCP_IO_REG);
    (this->*writesize[size])(*Bus::rdram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp mem

void MPMemory::read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(Bus::rcp->sp, address, dest);
}

void MPMemory::write_rsp_mem(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(Bus::rcp->sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp reg

void MPMemory::read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_REG);
    (this->*readsize[size])(Bus::rcp->sp, address, dest);
}

void MPMemory::write_rsp_reg(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_REG);
    (this->*writesize[size])(Bus::rcp->sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp stat

void MPMemory::read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_STAT);
    (this->*readsize[size])(Bus::rcp->sp, address, dest);
}

void MPMemory::write_rsp_stat(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp->sp.setIOMode(RCP_IO_STAT);
    (this->*writesize[size])(Bus::rcp->sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dpc

void MPMemory::read_dp(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->dpc, address, dest);
}

void MPMemory::write_dp(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->dpc, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dps

void MPMemory::read_dps(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->dps, address, dest);
}

void MPMemory::write_dps(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->dps, address, src);
}

//////////////////////////////////////////////////////////////////////////
// mi

void MPMemory::read_mi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->mi, address, dest);
}

void MPMemory::write_mi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->mi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// vi

void MPMemory::read_vi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->vi, address, dest);
}

void MPMemory::write_vi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->vi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ai

void MPMemory::read_ai(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->ai, address, dest);
}

void MPMemory::write_ai(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->ai, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pi

void MPMemory::read_pi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->pi, address, dest);
}

void MPMemory::write_pi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->pi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ri

void MPMemory::read_ri(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->ri, address, dest);
}

void MPMemory::write_ri(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->ri, address, src);
}

//////////////////////////////////////////////////////////////////////////
// si

void MPMemory::read_si(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp->si, address, dest);
}

void MPMemory::write_si(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp->si, address, src);
}

//////////////////////////////////////////////////////////////////////////
// flashram

void MPMemory::read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*Bus::flashram, address, dest);
}

void MPMemory::write_flashram_dummy(uint32_t address, uint64_t src, DataSize size)
{
}

void MPMemory::write_flashram_command(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*Bus::flashram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rom

void MPMemory::read_rom(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*Bus::rom, address, dest);
}

void MPMemory::write_rom(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*Bus::rom, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pif

void MPMemory::read_pif(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*Bus::pif, address, dest);
}

void MPMemory::write_pif(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*Bus::pif, address, src);
}

void MPMemory::unprotectFramebuffer(void)
{
    if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite &&
        fbInfo[0].addr)
    {
        vec_for(uint32_t i = 0; i < 6; i++)
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
}

void MPMemory::protectFramebuffer(void)
{
    if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite)
    {
        Bus::plugins->gfx()->fbGetInfo(fbInfo);
    }

    if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite
        && fbInfo[0].addr)
    {
        vec_for(uint32_t i = 0; i < 6; i++)
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


