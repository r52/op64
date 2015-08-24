#include "dpsinterface.h"
#include "oputil.h"

OPStatus DPSInterface::read(Bus* bus, uint32_t address, uint32_t* data)
{
    *data = reg[DPS_REG(address)];
    return OP_OK;
}

OPStatus DPSInterface::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = DPS_REG(address);

    masked_write(&reg[regnum], data, mask);
    return OP_OK;
}
