#include "rdraminterface.h"
#include "util.h"

OPStatus RDRAMInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[RI_REG(address)];

    return OP_OK;
}

OPStatus RDRAMInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = RI_REG(address);

    masked_write(&reg[regnum], data, mask);
    return OP_OK;
}
