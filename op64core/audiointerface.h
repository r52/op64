#pragma once

#include "rcpinterface.h"
#include "registerinterface.h"

struct ai_dma
{
    uint32_t length;
    unsigned int delay;
};

class AudioInterface : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(uint32_t address, uint32_t* data) override;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) override;

public:
    ai_dma fifo[2];
};
