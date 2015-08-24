#pragma once

#include "rcpinterface.h"
#include "registerinterface.h"

class SerialInterface : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

private:
    void DMARead(Bus* bus);
    void DMAWrite(Bus* bus);
};
