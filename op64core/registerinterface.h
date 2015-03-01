#pragma once

#include "op64.h"
#include "rcpcommon.h"

class RegisterInterface
{
public:
    uint32_t reg[RCP_MAX_NUM_REGS];
};
