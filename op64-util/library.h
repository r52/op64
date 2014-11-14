#pragma once

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
