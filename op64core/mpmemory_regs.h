#pragma once

#define COMMON_REG(x) ((x & 0xffff) >> 2)

#define RDRAM_ADDRESS(x) ((x & 0xffffff) >> 2)
#define RDRAM_REG(x) ((x & 0x3ff) >> 2)

#define RSP_ADDRESS(x) ((x & 0x1fff) >> 2)
#define RSP_REG(x) COMMON_REG(x)
#define RSP_REG2(x) COMMON_REG(x)

#define DPC_REG(x) COMMON_REG(x)
#define DPS_REG(x) COMMON_REG(x)

#define MI_REG(x) COMMON_REG(x)
#define VI_REG(x) COMMON_REG(x)
#define AI_REG(x) COMMON_REG(x)
#define PI_REG(x) COMMON_REG(x)
#define RI_REG(x) COMMON_REG(x)
#define SI_REG(x) COMMON_REG(x)

#define ROM_ADDRESS(x) (x & 0x03fffffc)

#define PIF_ADDRESS(x) ((x & 0xfffc) - 0x7c0)
