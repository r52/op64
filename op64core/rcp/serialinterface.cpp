#include <oputil.h>

#include "serialinterface.h"

#include <rcp/rcp.h>
#include <core/bus.h>
#include <cpu/dma.h>
#include <cpu/interrupthandler.h>

OPStatus SerialInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[SI_REG(address)];
    return OP_OK;
}

OPStatus SerialInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = SI_REG(address);

    switch (regnum)
    {
    case SI_DRAM_ADDR_REG:
        masked_write(&reg[SI_DRAM_ADDR_REG], data, mask);
        break;

    case SI_PIF_ADDR_RD64B_REG:
        masked_write(&reg[SI_PIF_ADDR_RD64B_REG], data, mask);
        DMA::readSI();
        break;

    case SI_PIF_ADDR_WR64B_REG:
        masked_write(&reg[SI_PIF_ADDR_WR64B_REG], data, mask);
        DMA::writeSI();
        break;

    case SI_STATUS_REG:
        reg[SI_STATUS_REG] &= ~0x1000;
        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x2;
        Bus::interrupt->checkInterrupt();
        break;
    }

    return OP_OK;
}
