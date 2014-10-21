#include "optime.h"

namespace op
{
    tm localtime(const std::time_t& time)
    {
        std::tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
        localtime_s(&tm_snapshot, &time);
#else
        localtime_r(&time, &tm_snapshot); // POSIX  
#endif
        return tm_snapshot;
    }
}
