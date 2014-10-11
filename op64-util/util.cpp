#include "util.h"
#include "logger.h"

#ifndef _MSC_VER
#include <dlfcn.h>
#endif


bool opLoadLib(LibHandle* handle, const char* libpath)
{
    if (handle == nullptr || libpath == nullptr || strlen(libpath) == 0)
    {
        LOG_ERROR("opLoadLib: No library specified");
        return false;
    }

#ifdef _MSC_VER
    const size_t size = strlen(libpath) + 1;
    std::wstring path(size, L'#');
    mbstowcs(&path[0], libpath, size);

    *handle = LoadLibrary(path.c_str());

    if (*handle == nullptr)
    {
        char *pchErrMsg;
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pchErrMsg, 0, NULL);
        LOG_ERROR("opLoadLib('%s') error: %s", libpath, pchErrMsg);
        LocalFree(pchErrMsg);
        return false;
    }
#else
    *handle = dlopen(libpath, RTLD_NOW);

    if (*handle == nullptr)
    {
        LOG_ERROR("dlopen('%s') failed: %s", libpath, dlerror());
        return false;
    }
#endif

    return true;
}

void* opLibGetFunc(LibHandle lib, const char* procname)
{
    if (procname == nullptr)
        return nullptr;
#ifdef _MSC_VER
    return GetProcAddress(lib, procname);
#else
    return dlsym(lib, procname);
#endif
}

bool opLibClose(LibHandle lib)
{
#ifdef _MSC_VER
    int rval = FreeLibrary(lib);

    if (rval == 0)
    {
        char *pchErrMsg;
        DWORD dwErr = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pchErrMsg, 0, NULL);
        LOG_ERROR("FreeLibrary() error: %s", pchErrMsg);
        LocalFree(pchErrMsg);
        return false;
    }
#else
    int rval = dlclose(lib);

    if (rval != 0)
    {
        LOG_ERROR("dlclose() failed: %s", dlerror());
        return false;
    }
#endif

    return true;
}
