#include "compiler.h"

// compiler messages
#define __STR2__(x) #x
#define __STR__(x) __STR2__(x)

#if defined(__cilk)
#pragma message ("Cilk Plus detected. Using Cilk Plus vectorizations")
#else
#pragma message ("Cilk Plus not detected. Falling back to standard library")
#endif

#define COMPILER_MESSAGE "Compiling with "

#if defined(__INTEL_COMPILER)
#pragma message COMPILER_MESSAGE "Intel C++ v" __STR__(__INTEL_COMPILER)
#elif defined(__clang__)
#pragma message COMPILER_MESSAGE "clang v" __STR__(CLANG_VERSION)
#elif defined(__GNUC__)
#pragma message COMPILER_MESSAGE "g++ v" __STR__(GCC_VERSION)
#elif defined(_MSC_VER)
#pragma message (COMPILER_MESSAGE "MSVC v" __STR__(_MSC_VER))
#endif

#ifndef HAS_CXX11_LIST_INST
#pragma message ("MSVC 2013 and lower does not fully support C++11. Falling back to ugly hacks")
#else
#pragma message ("C++11 support detected")
#endif

