#pragma once

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
    switch (Bus::cpu->rounding_mode) {
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
    return Bus::cpu->rounding_mode;
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

    pt = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
    _mm_store_ss(&result, pt);

    return result;
}

inline double i32_to_f64(int i)
{
    double result;
    __m128d pt;

    pt = _mm_cvtsi32_sd(_mm_setzero_pd(), i);
    _mm_store_sd(&result, pt);

    return result;
}

inline int32_t f64_to_i32(double* d)
{
    return _mm_cvtsd_si32(_mm_load_sd(d));
}

inline int64_t f64_to_i64(double* d)
{
    return _mm_cvtsd_si64(_mm_load_sd(d));
}

inline float i64_to_f32(int64_t i)
{
    float result;
    __m128 pt;

    pt = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
    _mm_store_ss(&result, pt);

    return result;
}

inline double i64_to_f64(int64_t i)
{
    double result;
    __m128d pt;

    pt = _mm_cvtsi64_sd(_mm_setzero_pd(), i);
    _mm_store_sd(&result, pt);

    return result;
}

inline double f32_to_f64(float* f)
{
    double result;
    __m128d pt;

    pt = _mm_cvtss_sd(_mm_setzero_pd(), _mm_load_ss(f));
    _mm_store_sd(&result, pt);

    return result;
}

inline float f64_to_f32(double* d)
{
    float result;
    __m128 fl;

    fl = _mm_cvtsd_ss(_mm_setzero_ps(), _mm_load_sd(d));
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

inline int64_t trunc_f32_to_i64(float* f)
{
    return _mm_cvttss_si64(_mm_load_ss(f));
}

inline int64_t trunc_f64_to_i64(double* f)
{
    return _mm_cvttsd_si64(_mm_load_sd(f));
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

inline float mul_f32(float* fs, float* ft)
{
    float result;
    __m128 fd;

    fd = _mm_mul_ss(_mm_load_ss(fs), _mm_load_ss(ft));
    _mm_store_ss(&result, fd);

    return result;
}

inline double mul_f64(double* fs, double* ft)
{
    double result;
    __m128d fd;

    fd = _mm_mul_sd(_mm_load_sd(fs), _mm_load_sd(ft));
    _mm_store_sd(&result, fd);

    return result;
}

inline float add_f32(float* fs, float* ft)
{
    float result;
    __m128 fd;

    fd = _mm_add_ss(_mm_load_ss(fs), _mm_load_ss(ft));
    _mm_store_ss(&result, fd);

    return result;
}

inline double add_f64(double* fs, double* ft)
{
    double result;
    __m128d fd;

    fd = _mm_add_sd(_mm_load_sd(fs), _mm_load_sd(ft));
    _mm_store_sd(&result, fd);

    return result;
}

inline float sub_f32(float* fs, float* ft)
{
    float result;
    __m128 fd;

    fd = _mm_sub_ss(_mm_load_ss(fs), _mm_load_ss(ft));
    _mm_store_ss(&result, fd);

    return result;
}

inline double sub_f64(double* fs, double* ft)
{
    double result;
    __m128d fd;

    fd = _mm_sub_sd(_mm_load_sd(fs), _mm_load_sd(ft));
    _mm_store_sd(&result, fd);

    return result;
}

inline float div_f32(float* fs, float* ft)
{
    float result;
    __m128 fd;

    fd = _mm_div_ss(_mm_load_ss(fs), _mm_load_ss(ft));
    _mm_store_ss(&result, fd);

    return result;
}

inline double div_f64(double* fs, double* ft)
{
    double result;
    __m128d fd;

    fd = _mm_div_sd(_mm_load_sd(fs), _mm_load_sd(ft));
    _mm_store_sd(&result, fd);

    return result;
}

inline float neg_f32(float* f)
{
    float result;
    __m128 fd;

    fd = _mm_xor_ps(_mm_set_ss(-0.0f), _mm_load_ss(f));
    _mm_store_ss(&result, fd);

    return result;
}

inline double neg_f64(double* f)
{
    double result;
    __m128d fd;

    fd = _mm_xor_pd(_mm_set_sd(-0.0f), _mm_load_sd(f));
    _mm_store_sd(&result, fd);

    return result;
}

inline float abs_f32(float* f)
{
    float result;
    __m128 fd;

    fd = _mm_andnot_ps(_mm_set_ss(-0.0f), _mm_load_ss(f));
    _mm_store_ss(&result, fd);

    return result;
}

inline double abs_f64(double* f)
{
    double result;
    __m128d fd;

    fd = _mm_andnot_pd(_mm_set_sd(-0.0f), _mm_load_sd(f));
    _mm_store_sd(&result, fd);

    return result;
}

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
extern "C" {
    void fpu_cmp_32(float* fs, float* ft);
    void fpu_cmp_64(double* fs, double* ft);
}
#endif

inline uint8_t c_cmp_32(float* fs, float* ft)
{
    // If using windows ICC, use inline asm
#if defined(_MSC_VER) && defined(__INTEL_COMPILER)
    __asm
    {
        movss xmm0, DWORD PTR[rcx]
        movss xmm1, DWORD PTR[rdx]
        comiss xmm0, xmm1
    }
#elif defined(_MSC_VER) && ! defined(__INTEL_COMPILER)
    fpu_cmp_32(fs, ft);
#else
    __m128 ffs;
    __m128 fft;

    ffs = _mm_load_ss(fs);
    fft = _mm_load_ss(ft);

    __asm__ __volatile__(
        "comiss %1, %2\n\t"
        : /* No outputs as we call readeflags */
        : "x" (ffs), "x" (fft)
        : "cc"
        );
#endif

    uint64_t flags = __readeflags();
    // ZF . PF . CF
    uint8_t result = (flags & 1) | ((flags >> 1) & 2) | ((flags >> 4) & 4);
    return result;
}

inline uint8_t c_cmp_64(double* fs, double* ft)
{
#if defined(_MSC_VER) && defined(__INTEL_COMPILER)
    __asm
    {
        movsd xmm0, QWORD PTR[rcx]
        movsd xmm1, QWORD PTR[rdx]
        comisd xmm0, xmm1
    }
#elif defined(_MSC_VER) && ! defined(__INTEL_COMPILER)
    fpu_cmp_64(fs, ft);
#else
    __m128d ffs;
    __m128d fft;

    ffs = _mm_load_sd(fs);
    fft = _mm_load_sd(ft);

    __asm__ __volatile__(
        "comisd %1, %2\n\t"
        : /* No outputs as we call readeflags */
        : "x" (ffs), "x" (fft)
        : "cc"
        );
#endif

    uint64_t flags = __readeflags();
    uint8_t result = (flags & 1) | ((flags >> 1) & 2) | ((flags >> 4) & 4);
    return result;
}

#pragma optimize("", on)

