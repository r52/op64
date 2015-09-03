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
#define _s_snprintf(buf, len, ...) _snprintf_s(buf, len, __VA_ARGS__)
#else
#define _s_snprintf(buf, len, ...) snprintf(buf, len, __VA_ARGS__)
#endif

#ifdef _MSC_VER
#define _s_sprintf(buf, len, ...) sprintf_s(buf, len, __VA_ARGS__)
#else
#define _s_sprintf(buf, len, ...) sprintf(buf, __VA_ARGS__)
#endif

#define fill_array(arr, start, len, obj) std::fill(arr + start, arr + start + len, obj)

// Check for c++11
#if ( defined(__GNUC__) && GCC_VERSION >= 40700 ) || \
    ( defined(__clang__) && CLANG_VERSION >= 30100) || \
    ( defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1500 ) || \
    ( (defined(_MSC_VER) && _MSC_VER > 1800) )
#define HAS_CXX11
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define __func__ __FUNCTION__
#endif

// compiler messages
#define __STR2__(x) #x
#define __STR__(x) __STR2__(x)

#if defined(__INTEL_COMPILER)
#define COMPILER "Intel C++ " __STR__(__VERSION__)
#elif defined(__clang__)
#define COMPILER "clang " __STR__(__clang_version__)
#elif defined(__GNUC__)
#define COMPILER "g++ " __STR__(__VERSION__)
#elif defined(_MSC_VER)
#define COMPILER __STR__(__VERSION__)
#endif

// Library
#if defined _MSC_VER || defined __CYGWIN__
#define OP_HELPER_DLL_IMPORT __declspec(dllimport)
#define OP_HELPER_DLL_EXPORT __declspec(dllexport)
#else
#define OP_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#define OP_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#endif

#ifdef OP_DLL_EXPORTS
#define OP_API OP_HELPER_DLL_EXPORT
#else
#define OP_API OP_HELPER_DLL_IMPORT
#endif

#define CACHE_LINE_SIZE 64
