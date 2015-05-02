#include <oputil.h>

#include "dpcinterface.h"

#include <core/bus.h>
#include <rcp/rcp.h>
#include <plugin/plugins.h>
#include <plugin/gfxplugin.h>
#include <cpu/interrupthandler.h>

OPStatus DPCInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[DPC_REG(address)];
    return OP_OK;
}

OPStatus DPCInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = DPC_REG(address);

    switch (regnum)
    {
    case DPC_STATUS_REG:
        if (updateDPC(data & mask))
            Bus::rcp->sp.prepareRSP();
    case DPC_CURRENT_REG:
    case DPC_CLOCK_REG:
    case DPC_BUFBUSY_REG:
    case DPC_PIPEBUSY_REG:
    case DPC_TMEM_REG:
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    switch (regnum)
    {
    case DPC_START_REG:
        reg[DPC_CURRENT_REG] = reg[DPC_START_REG];
        break;
    case DPC_END_REG:
        Bus::plugins->gfx()->ProcessRDPList();
        Bus::rcp->mi.reg[MI_INTR_REG] |= 0x20;
        Bus::interrupt->checkInterrupt();
        break;
    }

    return OP_OK;
}

bool DPCInterface::updateDPC(uint32_t w)
{
    bool do_sp_task_on_unfreeze = false;

    if (w & 0x1) // clear xbus_dmem_dma
        reg[DPC_STATUS_REG] &= ~0x1;

    if (w & 0x2) // set xbus_dmem_dma
        reg[DPC_STATUS_REG] |= 0x1;

    if (w & 0x4) // clear freeze
    {
        reg[DPC_STATUS_REG] &= ~0x2;

        // see do_SP_task for more info
        if (!(Bus::rcp->sp.reg[SP_STATUS_REG] & 0x3)) // !halt && !broke
        {
            do_sp_task_on_unfreeze = true;
        }
    }

    if (w & 0x8) // set freeze
        reg[DPC_STATUS_REG] |= 0x2;

    if (w & 0x10) // clear flush
        reg[DPC_STATUS_REG] &= ~0x4;

    if (w & 0x20) // set flush
        reg[DPC_STATUS_REG] |= 0x4;

    return do_sp_task_on_unfreeze;
}
