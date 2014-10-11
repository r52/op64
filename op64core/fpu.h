#pragma once

#include "bus.h"
#include "icpu.h"
#include "cp1.h"

#include <cfenv>
#include <xmmintrin.h>


enum CompareFlag
{
    CMP_GREATER_THAN = 0,
    CMP_LESS_THAN = 1,
    CMP_EQUAL = 4,
    CMP_UNORDERED = 7
};

#pragma  optimize("", off)

inline void set_rounding(void)
{
    switch (Bus::cpu->getcp1()->rounding_mode) {
    case ROUND_MODE:
        fesetround(FE_TONEAREST);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
        break;
    case TRUNC_MODE:
        fesetround(FE_TOWARDZERO);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
        break;
    case CEIL_MODE:
        fesetround(FE_UPWARD);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
        break;
    case FLOOR_MODE:
        fesetround(FE_DOWNWARD);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
        break;
    }
}

inline void set_rounding(uint32_t mode)
{
    switch (mode) {
    case ROUND_MODE:
        fesetround(FE_TONEAREST);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
        break;
    case TRUNC_MODE:
        fesetround(FE_TOWARDZERO);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_TOWARD_ZERO);
        break;
    case CEIL_MODE:
        fesetround(FE_UPWARD);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
        break;
    case FLOOR_MODE:
        fesetround(FE_DOWNWARD);
        _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
        break;
    }
}

inline uint32_t get_rounding(void)
{
    return Bus::cpu->getcp1()->rounding_mode;
}


inline int32_t f32_to_i32(float* f)
{
    return _mm_cvtss_si32(_mm_load_ss(f));
}

inline int64_t f32_to_i64(float* f)
{
    return _mm_cvtss_si64(_mm_load_ss(f));
}

inline float i32_to_f32(int i)
{
    float result;
    __m128 pt;

    pt = _mm_setzero_ps();
    pt = _mm_cvtsi32_ss(pt, i);
    _mm_store_ss(&result, pt);

    return result;
}

inline double i32_to_f64(int i)
{
    double result;
    __m128d pt;

    pt = _mm_setzero_pd();
    pt = _mm_cvtsi32_sd(pt, i);
    _mm_store_sd(&result, pt);

    return result;
}

inline int32_t f64_to_i32(double d)
{
    return _mm_cvtsd_si32(_mm_load_sd(&d));
}

inline int64_t f64_to_i64(double d)
{
    return _mm_cvtsd_si64(_mm_load_sd(&d));
}

inline float i64_to_f32(int64_t i)
{
    float result;
    __m128 pt;

    pt = _mm_setzero_ps();
    pt = _mm_cvtsi64_ss(pt, i);
    _mm_store_ss(&result, pt);

    return result;
}

inline double i64_to_f64(int64_t i)
{
    double result;
    __m128d pt;

    pt = _mm_setzero_pd();
    pt = _mm_cvtsi64_sd(pt, i);
    _mm_store_sd(&result, pt);

    return result;
}

inline double f32_to_f64(float* f)
{
    double result;
    __m128d pt;
    __m128 fl;

    pt = _mm_setzero_pd();
    fl = _mm_load_ss(f);
    pt = _mm_cvtss_sd(pt, fl);
    _mm_store_sd(&result, pt);

    return result;
}

inline float f64_to_f32(double* d)
{
    float result;
    __m128d pt;
    __m128 fl;

    fl = _mm_setzero_ps();
    pt = _mm_load_sd(d);
    fl = _mm_cvtsd_ss(fl, pt);
    _mm_store_ss(&result, fl);

    return result;
}

inline int32_t trunc_f32_to_i32(float* fs)
{
    return _mm_cvttss_si32(_mm_load_ss(fs));
}

inline int32_t trunc_f64_to_i32(double* f)
{
    return _mm_cvttsd_si32(_mm_load_sd(f));
}

inline int64_t trunc_f32_to_i64(float f)
{
    return _mm_cvttss_si64(_mm_load_ss(&f));
}

inline int64_t trunc_f64_to_i64(double f)
{
    return _mm_cvttsd_si64(_mm_load_sd(&f));
}

inline float sqrt_f32(float* f)
{
    float result;
    __m128 pt;

    pt = _mm_sqrt_ss(_mm_load_ss(f));
    _mm_store_ss(&result, pt);

    return result;
}

inline double sqrt_f64(double* f)
{
    double result;
    __m128d pt, fl;

    fl = _mm_load_sd(f);
    pt = _mm_sqrt_sd(fl, fl);
    _mm_store_sd(&result, pt);

    return result;
}

inline float mul_f32(float* ffs, float* fft)
{
    float result;
    __m128 fd, ft, fs;

    fs = _mm_load_ss(ffs);
    ft = _mm_load_ss(fft);
    fd = _mm_mul_ss(fs, ft);
    _mm_store_ss(&result, fd);

    return result;
}

inline double mul_f64(double* ffs, double* fft)
{
    double result;
    __m128d fd, ft, fs;

    fs = _mm_load_sd(ffs);
    ft = _mm_load_sd(fft);
    fd = _mm_mul_sd(fs, ft);
    _mm_store_sd(&result, fd);

    return result;
}

inline float add_f32(float* ffs, float* fft)
{
    float result;
    __m128 fd, ft, fs;

    fs = _mm_load_ss(ffs);
    ft = _mm_load_ss(fft);
    fd = _mm_add_ss(fs, ft);
    _mm_store_ss(&result, fd);

    return result;
}

inline double add_f64(double* ffs, double* fft)
{
    double result;
    __m128d fd, ft, fs;

    fs = _mm_load_sd(ffs);
    ft = _mm_load_sd(fft);
    fd = _mm_add_sd(fs, ft);
    _mm_store_sd(&result, fd);

    return result;
}

inline float sub_f32(float* ffs, float* fft)
{
    float result;
    __m128 fd, ft, fs;

    fs = _mm_load_ss(ffs);
    ft = _mm_load_ss(fft);
    fd = _mm_sub_ss(fs, ft);
    _mm_store_ss(&result, fd);

    return result;
}

inline double sub_f64(double* ffs, double* fft)
{
    double result;
    __m128d fd, ft, fs;

    fs = _mm_load_sd(ffs);
    ft = _mm_load_sd(fft);
    fd = _mm_sub_sd(fs, ft);
    _mm_store_sd(&result, fd);

    return result;
}

inline float div_f32(float* ffs, float* fft)
{
    float result;
    __m128 fd, ft, fs;

    fs = _mm_load_ss(ffs);
    ft = _mm_load_ss(fft);
    fd = _mm_div_ss(fs, ft);
    _mm_store_ss(&result, fd);

    return result;
}

inline double div_f64(double* ffs, double* fft)
{
    double result;
    __m128d fd, ft, fs;

    fs = _mm_load_sd(ffs);
    ft = _mm_load_sd(fft);
    fd = _mm_div_sd(fs, ft);
    _mm_store_sd(&result, fd);

    return result;
}

inline float neg_f32(float* f)
{
    float result;
    __m128 fd, fs;

    fs = _mm_load_ss(f);
    fd = _mm_xor_ps(_mm_set_ss(-0.0f), fs);
    _mm_store_ss(&result, fd);

    return result;
}

inline double neg_f64(double* f)
{
    double result;
    __m128d fd, fs;

    fs = _mm_load_sd(f);
    fd = _mm_xor_pd(_mm_set_sd(-0.0f), fs);
    _mm_store_sd(&result, fd);

    return result;
}

inline float abs_f32(float* f)
{
    float result;
    __m128 fd, fs;

    fs = _mm_load_ss(f);
    fd = _mm_andnot_ps(_mm_set_ss(-0.0f), fs);
    _mm_store_ss(&result, fd);

    return result;
}

inline double abs_f64(double* f)
{
    double result;
    __m128d fd, fs;

    fs = _mm_load_sd(f);
    fd = _mm_andnot_pd(_mm_set_sd(-0.0f), fs);
    _mm_store_sd(&result, fd);

    return result;
}

inline uint8_t c_cmp_32(float* fs, float* ft)
{
    // TODO future, linux version
    __asm
    {
        movss xmm0, DWORD PTR[rcx]
        movss xmm1, DWORD PTR[rdx]
        comiss xmm0, xmm1
    }

    uint64_t flags = __readeflags();
    uint8_t result = (flags & 1) | ((flags >> 1) & 2) | ((flags >> 4) & 4);
    return result;
}

inline uint8_t c_cmp_64(double* fs, double* ft)
{
    // TODO future, linux version
    __asm
    {
        movsd xmm0, QWORD PTR[rcx]
        movsd xmm1, QWORD PTR[rdx]
        comisd xmm0, xmm1
    }

    uint64_t flags = __readeflags();
    uint8_t result = (flags & 1) | ((flags >> 1) & 2) | ((flags >> 4) & 4);
    return result;
}

#pragma optimize("", on)

