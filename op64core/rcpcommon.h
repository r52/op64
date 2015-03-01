#pragma once


enum
{
    AI_DRAM_ADDR_REG = 0,
    AI_LEN_REG,
    AI_CONTROL_REG,
    AI_STATUS_REG,
    AI_DACRATE_REG,
    AI_BITRATE_REG,
    AI_NUM_REGS
};

enum
{
    DPC_START_REG = 0,
    DPC_END_REG,
    DPC_CURRENT_REG,
    DPC_STATUS_REG,
    DPC_CLOCK_REG,
    DPC_BUFBUSY_REG,
    DPC_PIPEBUSY_REG,
    DPC_TMEM_REG,
    DPC_NUM_REGS
};

enum
{
    DPS_TBIST_REG = 0,
    DPS_TEST_MODE_REG,
    DPS_BUFTEST_ADDR_REG,
    DPS_BUFTEST_DATA_REG,
    DPS_NUM_REGS
};

enum
{
    MI_INIT_MODE_REG = 0,
    MI_VERSION_REG,
    MI_INTR_REG,
    MI_INTR_MASK_REG,
    MI_NUM_REGS
};

enum
{
    PI_DRAM_ADDR_REG = 0,
    PI_CART_ADDR_REG,
    PI_RD_LEN_REG,
    PI_WR_LEN_REG,
    PI_STATUS_REG,
    PI_BSD_DOM1_LAT_REG,
    PI_BSD_DOM1_PWD_REG,
    PI_BSD_DOM1_PGS_REG,
    PI_BSD_DOM1_RLS_REG,
    PI_BSD_DOM2_LAT_REG,
    PI_BSD_DOM2_PWD_REG,
    PI_BSD_DOM2_PGS_REG,
    PI_BSD_DOM2_RLS_REG,
    PI_NUM_REGS
};

enum
{
    RDRAM_CONFIG_REG = 0,
    RDRAM_DEVICE_ID_REG,
    RDRAM_DELAY_REG,
    RDRAM_MODE_REG,
    RDRAM_REF_INTERVAL_REG,
    RDRAM_REF_ROW_REG,
    RDRAM_RAS_INTERVAL_REG,
    RDRAM_MIN_INTERVAL_REG,
    RDRAM_ADDR_SELECT_REG,
    RDRAM_DEVICE_MANUF_REG,
    RDRAM_NUM_REGS
};

enum
{
    RI_MODE_REG = 0,
    RI_CONFIG_REG,
    RI_CURRENT_LOAD_REG,
    RI_SELECT_REG,
    RI_REFRESH_REG,
    RI_LATENCY_REG,
    RI_RERROR_REG,
    RI_WERROR_REG,
    RI_NUM_REGS
};


enum
{
    SI_DRAM_ADDR_REG = 0,
    SI_PIF_ADDR_RD64B_REG,
    _SI_RESERVED_1,
    _SI_RESERVED_2,
    SI_PIF_ADDR_WR64B_REG,
    _SI_RESERVED_3,
    SI_STATUS_REG,
    SI_NUM_REGS
};

enum
{
    SP_MEM_ADDR_REG = 0,
    SP_DRAM_ADDR_REG,
    SP_RD_LEN_REG,
    SP_WR_LEN_REG,
    SP_STATUS_REG,
    SP_DMA_FULL_REG,
    SP_DMA_BUSY_REG,
    SP_SEMAPHORE_REG,
    SP_NUM_REGS
};

enum
{
    SP_PC_REG = 0,
    SP_IBIST_REG,
    SP2_NUM_REGS
};

enum
{
    VI_STATUS_REG = 0,
    VI_ORIGIN_REG,
    VI_WIDTH_REG,
    VI_INTR_REG,
    VI_CURRENT_REG,
    VI_BURST_REG,
    VI_V_SYNC_REG,
    VI_H_SYNC_REG,
    VI_LEAP_REG,
    VI_H_START_REG,
    VI_V_START_REG,
    VI_V_BURST_REG,
    VI_X_SCALE_REG,
    VI_Y_SCALE_REG,
    VI_NUM_REGS
};

// Memory address to register macros
#define COMMON_REG(addr) ((addr & 0xffff) >> 2)

#define RDRAM_ADDRESS(addr) ((addr & 0xffffff) >> 2)
#define RDRAM_REG(addr) ((addr & 0x3ff) >> 2)

#define RSP_ADDRESS(addr) ((addr & 0x1fff) >> 2)
#define RSP_REG(addr) COMMON_REG(addr)
#define RSP_REG2(addr) COMMON_REG(addr)

#define DPC_REG(addr) COMMON_REG(addr)
#define DPS_REG(addr) COMMON_REG(addr)

#define MI_REG(addr) COMMON_REG(addr)
#define VI_REG(addr) COMMON_REG(addr)
#define AI_REG(addr) COMMON_REG(addr)
#define PI_REG(addr) COMMON_REG(addr)
#define RI_REG(addr) COMMON_REG(addr)
#define SI_REG(addr) COMMON_REG(addr)

#define ROM_ADDRESS(addr) (addr & 0x03fffffc)

#define PIF_ADDRESS(addr) ((addr & 0xfffc) - 0x7c0)

// Largest is VI with 14 but whatever
#define RCP_MAX_NUM_REGS 16
