#pragma once

#include <cstdint>

enum {
    ROUND_MODE = 0x33F,
    FLOOR_MODE = 0x73F,
    CEIL_MODE = 0xB3F,
    TRUNC_MODE = 0xF3F
};


class CP1
{
public: 
    CP1(void);
    ~CP1(void);

    void shuffle_fpr_data(int oldStatus, int newStatus);
    void set_fpr_pointers(int newStatus);

    int32_t rounding_mode;

private:

    // registers
    uint32_t _FCR0, _FCR31;
    float* _s_reg[32];
    double* _d_reg[32];
    uint64_t _fgr[32];
};