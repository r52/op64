#include "rdramcontroller.h"
#include "oputil.h"



OPStatus RDRAMController::read(uint32_t address, uint32_t* data)
{
    switch (_iomode)
    {
    case RCP_IO_REG:
        return readReg(address, data);
        break;
    default:
        return readMem(address, data);
        break;
    }
    
    return OP_OK;
}

OPStatus RDRAMController::write(uint32_t address, uint32_t data, uint32_t mask)
{
    switch (_iomode)
    {
    case RCP_IO_REG:
        return writeReg(address, data, mask);
        break;
    default:
        return writeMem(address, data, mask);
        break;
    }
}

OPStatus RDRAMController::readMem(uint32_t address, uint32_t* data)
{
    *data = mem[RDRAM_ADDRESS(address)];
    return OP_OK;
}

OPStatus RDRAMController::readReg(uint32_t address, uint32_t* data)
{
    *data = reg[RDRAM_REG(address)];
    return OP_OK;
}

OPStatus RDRAMController::writeMem(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = RDRAM_ADDRESS(address);

    masked_write(&mem[addr], data, mask);
    return OP_OK;
}

OPStatus RDRAMController::writeReg(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = RDRAM_REG(address);

    masked_write(&reg[regnum], data, mask);
    return OP_OK;
}
