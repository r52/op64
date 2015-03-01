#pragma once

#include "rcpinterface.h"
#include "registerinterface.h"

class RDRAMInterface : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(uint32_t address, uint32_t* data) override;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) override;
};