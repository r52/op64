#include "rspinterface.h"
#include "util.h"

#include "bus.h"
#include "rcp.h"
#include "plugins.h"
#include "gfxplugin.h"
#include "rspplugin.h"
#include "interrupthandler.h"
#include "dma.h"
#include "icpu.h"
#include "cp0.h"
#include "mpmemory.h"

OPStatus RSPInterface::read(uint32_t address, uint32_t* data)
{
    switch (_iomode)
    {
    case RCP_IO_REG:
        return readReg(address, data);
        break;
    case RCP_IO_MEM:
        return readMem(address, data);
        break;
    case RCP_IO_STAT:
        return readStat(address, data);
        break;
    }

    return OP_OK;
}

OPStatus RSPInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    switch (_iomode)
    {
    case RCP_IO_REG:
        return writeReg(address, data, mask);
        break;
    case RCP_IO_MEM:
        return writeMem(address, data, mask);
        break;
    case RCP_IO_STAT:
        return writeStat(address, data, mask);
        break;
    }

    return OP_OK;
}

OPStatus RSPInterface::readMem(uint32_t address, uint32_t* data)
{
    *data = mem[RSP_ADDRESS(address)];
    return OP_OK;
}

OPStatus RSPInterface::readReg(uint32_t address, uint32_t* data)
{
    uint32_t regnum = RSP_REG(address);

    *data = reg[regnum];

    if (regnum == SP_SEMAPHORE_REG)
    {
        reg[SP_SEMAPHORE_REG] = 1;
    }

    return OP_OK;
}

OPStatus RSPInterface::readStat(uint32_t address, uint32_t* data)
{
    uint32_t reg = RSP_REG2(address);

    *data = stat[reg];
    return OP_OK;
}

OPStatus RSPInterface::writeMem(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = RSP_ADDRESS(address);

    masked_write(&mem[addr], data, mask);
    return OP_OK;
}

OPStatus RSPInterface::writeReg(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = RSP_REG(address);

    switch (regnum)
    {
    case SP_STATUS_REG:
        updateReg(data & mask);
    case SP_DMA_FULL_REG:
    case SP_DMA_BUSY_REG:
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    switch (regnum)
    {
    case SP_RD_LEN_REG:
        DMA::writeSP();
        break;
    case SP_WR_LEN_REG:
        DMA::readSP();
        break;
    case SP_SEMAPHORE_REG:
        reg[SP_SEMAPHORE_REG] = 0;
        break;
    }

    return OP_OK;
}

OPStatus RSPInterface::writeStat(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t reg = RSP_REG2(address);

    masked_write(&stat[reg], data, mask);
    return OP_OK;
}

void RSPInterface::prepareRSP(void)
{
    int32_t save_pc = stat[SP_PC_REG] & ~0xFFF;

    // Video task
    if (dmem[0xFC0 / 4] == 1)
    {
        if (Bus::rcp->dpc.reg[DPC_STATUS_REG] & 0x2) // DP frozen (DK64, BC)
        {
            // don't do the task now
            // the task will be done when DP is unfreezed (see update_DPC)
            return;
        }

        // unprotecting old frame buffers
        ((MPMemory*)Bus::mem)->unprotectFramebuffer();

        stat[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xffffffff);
        stat[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (Bus::rcp->mi.reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 1000);
        }

        if (Bus::rcp->mi.reg[MI_INTR_REG] & 0x20)
        {
            Bus::interrupt->addInterruptEvent(DP_INT, 1000);
        }

        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x21;
        reg[SP_STATUS_REG] &= ~0x303;

        // protecting new frame buffers
        ((MPMemory*)Bus::mem)->protectFramebuffer();
    }
    // Audio task
    else if (dmem[0xFC0 / 4] == 2)
    {
        stat[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        stat[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (Bus::rcp->mi.reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 4000/*500*/);
        }

        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x1;
        reg[SP_STATUS_REG] &= ~0x303;

    }
    // Unknown task
    else
    {
        stat[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        stat[SP_PC_REG] |= save_pc;

        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (Bus::rcp->mi.reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->addInterruptEvent(SP_INT, 0/*100*/);
        }

        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x1;
        reg[SP_STATUS_REG] &= ~0x203;
    }
}

void RSPInterface::updateReg(uint32_t w)
{
    if (w & 0x1) // clear halt
        reg[SP_STATUS_REG] &= ~0x1;
    if (w & 0x2) // set halt
        reg[SP_STATUS_REG] |= 0x1;

    if (w & 0x4) // clear broke
        reg[SP_STATUS_REG] &= ~0x2;

    if (w & 0x8) // clear SP interrupt
    {
        Bus::rcp->mi.reg[MI_INTR_REG] &= ~1;
        Bus::interrupt->checkInterrupt();
    }

    if (w & 0x10) // set SP interrupt
    {
        Bus::rcp->mi.reg[MI_INTR_REG] |= 1;
        Bus::interrupt->checkInterrupt();
    }

    if (w & 0x20) // clear single step
        reg[SP_STATUS_REG] &= ~0x20;
    if (w & 0x40) // set single step
        reg[SP_STATUS_REG] |= 0x20;

    if (w & 0x80) // clear interrupt on break
        reg[SP_STATUS_REG] &= ~0x40;
    if (w & 0x100) // set interrupt on break
        reg[SP_STATUS_REG] |= 0x40;

    if (w & 0x200) // clear signal 0
        reg[SP_STATUS_REG] &= ~0x80;
    if (w & 0x400) // set signal 0
        reg[SP_STATUS_REG] |= 0x80;

    if (w & 0x800) // clear signal 1
        reg[SP_STATUS_REG] &= ~0x100;
    if (w & 0x1000) // set signal 1
        reg[SP_STATUS_REG] |= 0x100;

    if (w & 0x2000) // clear signal 2
        reg[SP_STATUS_REG] &= ~0x200;
    if (w & 0x4000) // set signal 2
        reg[SP_STATUS_REG] |= 0x200;

    if (w & 0x8000) // clear signal 3
        reg[SP_STATUS_REG] &= ~0x400;
    if (w & 0x10000) // set signal 3
        reg[SP_STATUS_REG] |= 0x400;

    if (w & 0x20000) // clear signal 4
        reg[SP_STATUS_REG] &= ~0x800;
    if (w & 0x40000) // set signal 4
        reg[SP_STATUS_REG] |= 0x800;

    if (w & 0x80000) // clear signal 5
        reg[SP_STATUS_REG] &= ~0x1000;
    if (w & 0x100000) // set signal 5
        reg[SP_STATUS_REG] |= 0x1000;

    if (w & 0x200000) // clear signal 6
        reg[SP_STATUS_REG] &= ~0x2000;
    if (w & 0x400000) // set signal 6
        reg[SP_STATUS_REG] |= 0x2000;

    if (w & 0x800000) // clear signal 7
        reg[SP_STATUS_REG] &= ~0x4000;
    if (w & 0x1000000) // set signal 7
        reg[SP_STATUS_REG] |= 0x4000;

    //if (get_event(SP_INT)) return;
    if (!(w & 0x1) && !(w & 0x4))
        return;

    if (!(reg[SP_STATUS_REG] & 0x3)) // !halt && !broke
    {
        prepareRSP();
    }
}