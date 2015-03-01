#include "peripheralinterface.h"
#include "util.h"

#include "bus.h"
#include "rcp.h"
#include "dma.h"
#include "interrupthandler.h"

OPStatus PeripheralInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[PI_REG(address)];
    return OP_OK;
}

OPStatus PeripheralInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = PI_REG(address);

    switch (regnum)
    {
    case PI_RD_LEN_REG:
        masked_write(&reg[PI_RD_LEN_REG], data, mask);
        DMA::readPI();
        return OP_OK;

    case PI_WR_LEN_REG:
        masked_write(&reg[PI_WR_LEN_REG], data, mask);
        DMA::writePI();
        return OP_OK;

    case PI_STATUS_REG:
        if (data & mask & 2)
        {
            Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x10;
            Bus::interrupt->checkInterrupt();
        }

        return OP_OK;

    case PI_BSD_DOM1_LAT_REG:
    case PI_BSD_DOM1_PWD_REG:
    case PI_BSD_DOM1_PGS_REG:
    case PI_BSD_DOM1_RLS_REG:
    case PI_BSD_DOM2_LAT_REG:
    case PI_BSD_DOM2_PWD_REG:
    case PI_BSD_DOM2_PGS_REG:
    case PI_BSD_DOM2_RLS_REG:
        masked_write(&reg[regnum], data & 0xff, mask);
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);
    
    return OP_OK;
}
