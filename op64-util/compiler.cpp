#include "compiler.h"


#if defined(__cilk)
#pragma message ("Cilk Plus detected. Using Cilk Plus vectorizations")
#else
#pragma message ("Cilk Plus not detected. Not using vector optimizations")
#endif

#define COMPILER_MESSAGE "Compiling with "

#pragma message (COMPILER_MESSAGE COMPILER)

#ifndef HAS_CXX11_LIST_INST
#pragma message ("MSVC 2013 and lower does not fully support C++11. Falling back to ugly workarounds")
#else
#pragma message ("C++11 support detected")
#endif

