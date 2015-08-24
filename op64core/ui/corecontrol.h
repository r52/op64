#pragma once

#include <cstdint>
#include <atomic>

class CoreControl
{
public:
    static std::atomic<bool> stop;
    static std::atomic<bool> doHardReset;
    static std::atomic<bool> limitVI;
    static std::atomic<double> fps;
    static uint32_t VIRefreshRate;
};
