#include "cp1.h"
#include "bus.h"
#include "compiler.h"

#ifdef BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

CP1::CP1(void)
{
    using namespace Bus;
    FCR0 = &_FCR0;
    FCR31 = &_FCR31;
    s_reg = _s_reg;
    d_reg = _d_reg;
    fgr = _fgr;
}

CP1::~CP1(void)
{
    using namespace Bus;
    FCR0 = nullptr;
    FCR31 = nullptr;
    s_reg = nullptr;
    d_reg = nullptr;
    fgr = nullptr;
}

void CP1::shuffle_fpr_data(int oldStatus, int newStatus)
{
    if ((newStatus & 0x04000000) != (oldStatus & 0x04000000))
    {
        int32_t i;
        int32_t temp_fgr_32[32];

        // pack or unpack the FGR register data
        if (newStatus & 0x04000000)
        {   // switching into 64-bit mode
            // retrieve 32 FPR values from packed 32-bit FGR registers
            vec_for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int32_t *)&_fgr[i >> 1] + ((i & 1) ^ IS_BIG_ENDIAN));
            }
            // unpack them into 32 64-bit registers, taking the high 32-bits from their temporary place in the upper 16 FGRs
            vec_for (i = 0; i < 32; i++)
            {
                int32_t high32 = *((int32_t *)&_fgr[(i >> 1) + 16] + (i & 1));
                *((int32_t *)&_fgr[i] + IS_BIG_ENDIAN) = temp_fgr_32[i];
                *((int32_t *)&_fgr[i] + (IS_BIG_ENDIAN ^ 1)) = high32;
            }
        }
        else
        {   // switching into 32-bit mode
            // retrieve the high 32 bits from each 64-bit FGR register and store in temp array
            vec_for (i = 0; i < 32; i++)
            {
                temp_fgr_32[i] = *((int32_t *)&_fgr[i] + (IS_BIG_ENDIAN ^ 1));
            }
            // take the low 32 bits from each register and pack them together into 64-bit pairs
            vec_for (i = 0; i < 16; i++)
            {
                uint32_t least32 = *((uint32_t *)&_fgr[i * 2] + IS_BIG_ENDIAN);
                uint32_t most32 = *((uint32_t *)&_fgr[i * 2 + 1] + IS_BIG_ENDIAN);
                _fgr[i] = ((uint64_t) most32 << 32) | (uint64_t) least32;
            }
            // store the high bits in the upper 16 FGRs, which wont be accessible in 32-bit mode
            vec_for (i = 0; i < 32; i++)
            {
                *((int *)&_fgr[(i >> 1) + 16] + (i & 1)) = temp_fgr_32[i];
            }
        }
    }
}

void CP1::set_fpr_pointers(int newStatus)
{
    // update the FPR register pointers
    if (newStatus & 0x04000000)
    {
        vec_for(int32_t i = 0; i < 32; i++)
        {
            _d_reg[i] = (double*)&_fgr[i];
            _s_reg[i] = ((float*)&_fgr[i]) + IS_BIG_ENDIAN;
        }
    }
    else
    {
        vec_for(int32_t i = 0; i < 32; i++)
        {
            _d_reg[i] = (double*)&_fgr[i >> 1];
            _s_reg[i] = ((float*)&_fgr[i >> 1]) + ((i & 1) ^ IS_BIG_ENDIAN);
        }
    }
}
