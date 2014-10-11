#pragma once
#include <cstdint>

#ifdef _MSC_VER
#define __align(var, len) __declspec(align(len)) var
#elif (defined __GNUC__)
#define __align(var, len) var __attribute__ ((aligned(len)))
#else
#define __align(var, len) var len
#endif

#ifdef _MSC_VER
#define _safe_sprintf(buf, len, ...) sprintf_s(buf, __VA_ARGS__);
#else
#define _safe_sprintf(buf, len, ...) snprintf(buf, len, __VA_ARGS__);
#endif

// borrowed from cxd4/rsp
#ifdef BIG_ENDIAN
#define ENDIAN  0
#else
#define ENDIAN  ~0
#endif
#define BES(address)    ((address) ^ ((ENDIAN) & 03))
#define HES(address)    ((address) ^ ((ENDIAN) & 02))

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

#ifdef _MSC_VER
#include <windows.h>
#define IMPORT extern "C" __declspec(dllimport)
#define EXPORT __declspec(dllexport)
typedef HMODULE LibHandle;
#else
#define IMPORT extern "C"
#define EXPORT __attribute__((visibility("default")))
typedef void* LibHandle;
#endif

bool opLoadLib(LibHandle* handle, const char* libpath);
void* opLibGetFunc(LibHandle lib, const char* procname);
bool opLibClose(LibHandle lib);
