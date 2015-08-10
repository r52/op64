#pragma once

#include <cstdint>
#include <chrono>

enum TimingMode : uint8_t
{
    TIMING_NTSC = 0,
    TIMING_PAL
};

enum LimitMode : uint8_t
{
    LIMIT_BY_VI = 0,
    LIMIT_BY_AUDIO,
    LIMIT_BY_FREQ
};

class SysTiming
{
public:
    SysTiming() = default;
    SysTiming(uint32_t vilimit);

    void startTimers();

    void doVILimit();

    void setLimitMode(LimitMode mode)
    {
        _limitmode = mode;
        startTimers();
    }

private:
    uint64_t _framesElapsed = 0;
    TimingMode _mode = TIMING_NTSC;
    LimitMode _limitmode = LIMIT_BY_VI;

    std::chrono::high_resolution_clock::time_point _lastKeyVITime;
};
