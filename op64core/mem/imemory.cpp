#include "imemory.h"

#include <oplog.h>
#include <oputil.h>

#include <pif/pif.h>
#include <rom/sram.h>
#include <rom/flashram.h>
#include <plugin/plugincontainer.h>
#include <plugin/gfxplugin.h>

IMemory::~IMemory()
{
    uninitialize(_bus);
}

bool IMemory::initialize(Bus* bus)
{
    if (nullptr == bus)
    {
        LOG_ERROR(IMemory) << "Invalid Bus";
        return false;
    }

    _bus = bus;
    bus->sram.reset(new SRAM);
    bus->flashram.reset(new FlashRam);
    bus->pif.reset(new PIF);

    // init rdram
    fill_array(Bus::rdram.mem, 0, RDRAM_SIZE / 4, 0);

    // reset registers
    fill_array(Bus::rdram.reg, 0, RDRAM_NUM_REGS, 0);

    fill_array(Bus::rcp.sp.dmem, 0, (0x1000 / 4), 0);
    fill_array(Bus::rcp.sp.imem, 0, (0x1000 / 4), 0);

    fill_array(Bus::rcp.sp.reg, 0, SP_NUM_REGS, 0);
    Bus::rcp.sp.reg[SP_STATUS_REG] = 1;

    fill_array(Bus::rcp.sp.stat, 0, SP2_NUM_REGS, 0);

    fill_array(Bus::rcp.dpc.reg, 0, DPC_NUM_REGS, 0);

    fill_array(Bus::rcp.dps.reg, 0, DPS_NUM_REGS, 0);

    fill_array(Bus::rcp.mi.reg, 0, MI_NUM_REGS, 0);
    Bus::rcp.mi.reg[MI_VERSION_REG] = 0x02020102;

    fill_array(Bus::rcp.vi.reg, 0, VI_NUM_REGS, 0);

    fill_array(Bus::rcp.ai.reg, 0, AI_NUM_REGS, 0);

    fill_array(Bus::rcp.pi.reg, 0, PI_NUM_REGS, 0);

    fill_array(Bus::rcp.ri.reg, 0, RI_NUM_REGS, 0);

    fill_array(Bus::rcp.si.reg, 0, SI_NUM_REGS, 0);

    fbInfo[0].addr = 0;

    Bus::rcp.ai.fifo[0].delay = 0;
    Bus::rcp.ai.fifo[1].delay = 0;

    Bus::rcp.ai.fifo[0].length = 0;
    Bus::rcp.ai.fifo[1].length = 0;

    return bus->pif->initialize();
}

void IMemory::uninitialize(Bus* bus)
{
    if (bus)
    {
        bus->pif.reset();
        bus->sram.reset();
        bus->flashram.reset();
        _bus = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
// sized r/w

void IMemory::read_size_byte(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = BSHIFT(address);
    device.read(_bus, address, &data);
    *dest = (data >> shift) & 0xff;
}

void IMemory::read_size_half(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    unsigned shift = HSHIFT(address);
    device.read(_bus, address, &data);
    *dest = (data >> shift) & 0xffff;
}

void IMemory::read_size_word(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data;
    device.read(_bus, address, &data);
    *dest = data;
}

void IMemory::read_size_dword(RCPInterface& device, uint32_t& address, uint64_t* dest)
{
    uint32_t data[2];
    device.read(_bus, address, &data[0]);
    device.read(_bus, address + 4, &data[1]);
    *dest = ((uint64_t)data[0] << 32) | data[1];
}

void IMemory::write_size_byte(RCPInterface& device, uint32_t address, uint64_t src)
{
    uint32_t shift = BSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xff << shift;

    device.write(_bus, address, data, mask);
}

void IMemory::write_size_half(RCPInterface& device, uint32_t address, uint64_t src)
{
    uint32_t shift = HSHIFT(address);
    uint32_t data = (uint32_t)src << shift;
    uint32_t mask = (uint32_t)0xffff << shift;

    device.write(_bus, address, data, mask);
}

void IMemory::write_size_word(RCPInterface& device, uint32_t address, uint64_t src)
{
    device.write(_bus, address, src, ~0U);
}

void IMemory::write_size_dword(RCPInterface& device, uint32_t address, uint64_t src)
{
    device.write(_bus, address, src >> 32, ~0U);
    device.write(_bus, address + 4, src, ~0U);
}

//////////////////////////////////////////////////////////////////////////
// nothing

void IMemory::read_nothing(uint32_t& address, uint64_t* dest, DataSize size)
{
    *dest = 0;
}

void IMemory::write_nothing(uint32_t address, uint64_t src, DataSize size)
{
    // does nothing
}

//////////////////////////////////////////////////////////////////////////
// nomem

void IMemory::read_nomem(uint32_t& address, uint64_t* dest, DataSize size)
{
    address = TLB::virtual_to_physical_address(_bus, address, TLB_READ);
    if (address == 0x00000000)
        return;

    readmem(address, dest, size);
}

void IMemory::write_nomem(uint32_t address, uint64_t src, DataSize size)
{
    address = TLB::virtual_to_physical_address(_bus, address, TLB_WRITE);
    if (address == 0x00000000) return;

    writemem(address, src, size);
}

//////////////////////////////////////////////////////////////////////////
// rdram

void IMemory::read_rdram(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rdram.setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(Bus::rdram, address, dest);
}

void IMemory::read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size)
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
                _bus->plugins->gfx()->fbRead(address);
                framebufferRead[(address & 0x7FFFFF) >> 12] = 0;
            }
        }
    }

    Bus::rdram.setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(Bus::rdram, address, dest);
}

void IMemory::write_rdram(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rdram.setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(Bus::rdram, address, src);
}

void IMemory::write_rdramFB(uint32_t address, uint64_t src, DataSize size)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        if (fbInfo[i].addr)
        {
            uint32_t start = fbInfo[i].addr & 0x7FFFFF;
            uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
            if ((address & 0x7FFFFF) >= start && (address & 0x7FFFFF) <= end)
            {
                _bus->plugins->gfx()->fbWrite(address, 4);
            }
        }
    }

    Bus::rdram.setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(Bus::rdram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rdram reg

void IMemory::read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rdram.setIOMode(RCP_IO_REG);
    (this->*readsize[size])(Bus::rdram, address, dest);
}

void IMemory::write_rdram_reg(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rdram.setIOMode(RCP_IO_REG);
    (this->*writesize[size])(Bus::rdram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp mem

void IMemory::read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_MEM);
    (this->*readsize[size])(Bus::rcp.sp, address, dest);
}

void IMemory::write_rsp_mem(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_MEM);
    (this->*writesize[size])(Bus::rcp.sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp reg

void IMemory::read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_REG);
    (this->*readsize[size])(Bus::rcp.sp, address, dest);
}

void IMemory::write_rsp_reg(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_REG);
    (this->*writesize[size])(Bus::rcp.sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rsp stat

void IMemory::read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_STAT);
    (this->*readsize[size])(Bus::rcp.sp, address, dest);
}

void IMemory::write_rsp_stat(uint32_t address, uint64_t src, DataSize size)
{
    Bus::rcp.sp.setIOMode(RCP_IO_STAT);
    (this->*writesize[size])(Bus::rcp.sp, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dpc

void IMemory::read_dp(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.dpc, address, dest);
}

void IMemory::write_dp(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.dpc, address, src);
}

//////////////////////////////////////////////////////////////////////////
// dps

void IMemory::read_dps(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.dps, address, dest);
}

void IMemory::write_dps(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.dps, address, src);
}

//////////////////////////////////////////////////////////////////////////
// mi

void IMemory::read_mi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.mi, address, dest);
}

void IMemory::write_mi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.mi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// vi

void IMemory::read_vi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.vi, address, dest);
}

void IMemory::write_vi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.vi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ai

void IMemory::read_ai(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.ai, address, dest);
}

void IMemory::write_ai(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.ai, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pi

void IMemory::read_pi(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.pi, address, dest);
}

void IMemory::write_pi(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.pi, address, src);
}

//////////////////////////////////////////////////////////////////////////
// ri

void IMemory::read_ri(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.ri, address, dest);
}

void IMemory::write_ri(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.ri, address, src);
}

//////////////////////////////////////////////////////////////////////////
// si

void IMemory::read_si(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(Bus::rcp.si, address, dest);
}

void IMemory::write_si(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(Bus::rcp.si, address, src);
}

//////////////////////////////////////////////////////////////////////////
// flashram

void IMemory::read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*_bus->flashram, address, dest);
}

void IMemory::write_flashram_dummy(uint32_t address, uint64_t src, DataSize size)
{
}

void IMemory::write_flashram_command(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*_bus->flashram, address, src);
}

//////////////////////////////////////////////////////////////////////////
// rom

void IMemory::read_rom(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*_bus->rom, address, dest);
}

void IMemory::write_rom(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*_bus->rom, address, src);
}

//////////////////////////////////////////////////////////////////////////
// pif

void IMemory::read_pif(uint32_t& address, uint64_t* dest, DataSize size)
{
    (this->*readsize[size])(*_bus->pif, address, dest);
}

void IMemory::write_pif(uint32_t address, uint64_t src, DataSize size)
{
    (this->*writesize[size])(*_bus->pif, address, src);
}

