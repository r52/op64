#include "dpsinterface.h"
#include "oputil.h"

OPStatus DPSInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[DPS_REG(address)];
    return OP_OK;
}

OPStatus DPSInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = DPS_REG(address);

    masked_write(&reg[regnum], data, mask);
    return OP_OK;
}
