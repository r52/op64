#pragma once

#ifdef __clang__
#define CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifdef _MSC_VER
#define __align(var, len) __declspec(align(len)) var
#elif defined(__GNUC__)
#define __align(var, len) var __attribute__ ((aligned(len)))
#else
#define __align(var, len) var len
#endif

#ifdef _MSC_VER
#define _safe_sprintf(buf, len, ...) _snprintf_s(buf, len, __VA_ARGS__);
#else
#define _safe_sprintf(buf, len, ...) snprintf(buf, len, __VA_ARGS__);
#endif

#if defined(__cilk)
#include <cilk/cilk.h>
#define fill_array(arr, start, len, obj) arr[start:len] = obj
#define vec_for cilk_for
#else
#define fill_array(arr, start, len, obj) std::fill(arr + start, arr + start + len, obj)
#define vec_for for
#endif

#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER > 1800)
// If the compiler supports list instantiation *cough* MSVC >:(
#define HAS_CXX11_LIST_INST
#endif

#if defined(_MSC_VER) && ! defined(__INTEL_COMPILER)
#define __func__ __FUNCTION__
#endif
