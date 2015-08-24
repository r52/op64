#pragma once

#include <cstdint>

#include <tlb/tlb.h>

class ICPU;

class CP0
{
public:
    CP0(void) = default;
    ~CP0(void) = default;

    void updateCount(uint32_t PC, uint8_t countPerOp);
    bool COP1Unusable(ICPU& cpu);

    tlb_o tlb;

    uint32_t page_mask[32];
    uint32_t pfn[32][2];
    uint8_t state[32][2];
};
