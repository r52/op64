#pragma once

#include "oppreproc.h"

#ifdef _MSC_VER
#include <windows.h>
typedef HMODULE LibHandle;
#else
typedef void* LibHandle;
#endif

OP_API bool opLoadLib(LibHandle* handle, const char* libpath);
OP_API void* opLibGetFunc(LibHandle lib, const char* procname);
OP_API void* opLibGetMainHandle(void);
OP_API bool opLibClose(LibHandle lib);
