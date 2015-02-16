#pragma once

#include <cstdint>

#include "rcpcommon.h"

struct ai_dma
{
    uint32_t length;
    unsigned int delay;
};

class AiController
{
public:
    uint32_t reg[AI_NUM_REGS];
    ai_dma fifo[2];
};
