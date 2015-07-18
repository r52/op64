#pragma once

#include <cstdint>

#include <tlb/tlb.h>

enum CP0Regs : uint8_t
{
    CP0_INDEX_REG = 0,
    CP0_RANDOM_REG,
    CP0_ENTRYLO0_REG,
    CP0_ENTRYLO1_REG,
    CP0_CONTEXT_REG,
    CP0_PAGEMASK_REG,
    CP0_WIRED_REG,
    /* 7 is unused */
    CP0_BADVADDR_REG = 8,
    CP0_COUNT_REG,
    CP0_ENTRYHI_REG,
    CP0_COMPARE_REG,
    CP0_STATUS_REG,
    CP0_CAUSE_REG,
    CP0_EPC_REG,
    CP0_PREVID_REG,
    CP0_CONFIG_REG,
    CP0_LLADDR_REG,
    CP0_WATCHLO_REG,
    CP0_WATCHHI_REG,
    CP0_XCONTEXT_REG,
    /* 21 - 27 are unused */
    CP0_TAGLO_REG = 28,
    CP0_TAGHI_REG,
    CP0_ERROREPC_REG,
    /* 31 is unused */
    CP0_REGS_COUNT = 32
};

class CP0
{
public:
    CP0(void);
    ~CP0(void);

    void updateCount(uint32_t PC);
    bool COP1Unusable(void);

    tlb_o tlb;

    uint32_t page_mask[32];
    uint32_t pfn[32][2];
    uint8_t state[32][2];

private:
    uint32_t _cp0_reg[CP0_REGS_COUNT];
};
