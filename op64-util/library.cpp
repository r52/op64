#include "library.h"
#include "logger.h"

#ifndef _MSC_VER
#include <dlfcn.h>
#else
#include <strsafe.h>
#endif

#ifdef _MSC_VER
// Wide to Multibyte string convert from https://stackoverflow.com/questions/5513718/how-do-i-convert-from-lpctstr-to-stdstring
static std::string MBFromW(LPCWSTR pwsz, UINT cp) {
    int cch = WideCharToMultiByte(cp, 0, pwsz, -1, 0, 0, NULL, NULL);

    char* psz = new char[cch];

    WideCharToMultiByte(cp, 0, pwsz, -1, psz, cch, NULL, NULL);

    std::string st(psz);
    delete[] psz;

    return st;
}
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
    size_t convnum = 0;

    mbstowcs_s(&convnum, &path[0], size, libpath, _TRUNCATE);

    *handle = LoadLibrary(path.c_str());

    if (*handle == nullptr)
    {
        LPVOID lpMsgBuf;
        LPVOID lpDisplayBuf;
        DWORD dwErr = GetLastError();
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL);

        lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
            (lstrlen((LPCTSTR)lpMsgBuf) + 40) * sizeof(TCHAR));
        StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("error %d: %s"),
            dwErr, lpMsgBuf);

        std::string errMsg = MBFromW((LPCTSTR)lpDisplayBuf, CP_ACP);

        LOG_ERROR("opLoadLib('%s') %s", libpath, errMsg.c_str());
        LocalFree(lpMsgBuf);
        LocalFree(lpDisplayBuf);
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
