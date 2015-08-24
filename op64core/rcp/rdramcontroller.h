#pragma once

#include <oppreproc.h>

#include "rcpinterface.h"
#include "registerinterface.h"


#define RDRAM_SIZE 0x800000

class RDRAMController : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

private:
    OPStatus readMem(uint32_t address, uint32_t* data);
    OPStatus readReg(uint32_t address, uint32_t* data);

    OPStatus writeMem(uint32_t address, uint32_t data, uint32_t mask);
    OPStatus writeReg(uint32_t address, uint32_t data, uint32_t mask);

public:
    __align(uint32_t mem[RDRAM_SIZE / 4], 16);
};
