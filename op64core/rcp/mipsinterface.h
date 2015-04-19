#pragma once

#include "rcpinterface.h"
#include "registerinterface.h"

class MIPSInterface : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(uint32_t address, uint32_t* data) override;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) override;

private:
    bool update_mi_init_mode(uint32_t w);
    void update_mi_intr_mask(uint32_t w);
};
