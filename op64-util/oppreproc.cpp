#include "oppreproc.h"

#define COMPILER_MESSAGE "Compiling with "

#pragma message (COMPILER_MESSAGE COMPILER)

#ifndef HAS_CXX11

#ifdef _MSC_VER
#error "Visual Studio versions lower than 2015 is no longer supported. Please upgrade."
#else
#error "A compiler with strict C++11 support is required. Please upgrade your compiler."
#endif

#else
#pragma message ("C++11 support detected")
#endif

