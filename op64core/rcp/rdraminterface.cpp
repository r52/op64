#include "rdraminterface.h"
#include "oputil.h"

OPStatus RDRAMInterface::read(Bus* bus, uint32_t address, uint32_t* data)
{
    *data = reg[RI_REG(address)];

    return OP_OK;
}

OPStatus RDRAMInterface::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = RI_REG(address);

    masked_write(&reg[regnum], data, mask);
    return OP_OK;
}
