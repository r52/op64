#pragma once

#include <cstdint>

// compiler specific abstractions
#include "oppreproc.h"

// borrowed from cxd4/rsp
#ifdef BIG_ENDIAN
#define ENDIAN  0
#else
#define ENDIAN  ~0
#endif
#define BES(address)    ((address) ^ ((ENDIAN) & 03))
#define HES(address)    ((address) ^ ((ENDIAN) & 02))

#define BSHIFT(a) (((a & 3) ^ 3) << 3)
#define HSHIFT(a) (((a & 2) ^ 2) << 3)

#ifdef _MSC_VER
#include <intrin.h>
#endif

inline void masked_write(uint32_t* dst, uint32_t value, uint32_t mask)
{
    *dst = (*dst & ~mask) | (value & mask);
}

inline uint32_t byteswap_u32(uint32_t x) {
#ifdef BIG_ENDIAN
    return x;
#elif defined(_MSC_VER)
    return _byteswap_ulong(x);
#elif defined(__GNUC__)
    return __builtin_bswap32(x);
#else
    return
        (((((x) >> 24) & 0x000000FF) | \
        (((x) >> 8) & 0x0000FF00) | \
        (((x) << 8) & 0x00FF0000) | \
        (((x) << 24) & 0xFF000000));
#endif
}

template <typename F, typename T>
inline T signextend(const F x)
{
    return ((T)((F)x));
}
