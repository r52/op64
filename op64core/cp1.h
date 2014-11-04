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
    CP1(float* (*s_reg)[32], double* (*d_reg)[32], uint64_t (*fgr)[32]);
    ~CP1(void);

    void shuffle_fpr_data(int oldStatus, int newStatus);
    void set_fpr_pointers(int newStatus);

private:
    float* (*_s_reg)[32];
    double* (*_d_reg)[32];
    uint64_t (*_fgr)[32];
};
