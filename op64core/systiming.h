#pragma once

#include <cstdint>
#include <boost/chrono/chrono.hpp>

enum TimingMode
{
    TIMING_NTSC = 0,
    TIMING_PAL
};

enum LimitMode
{
    LIMIT_BY_VI = 0,
    LIMIT_BY_FREQ
};

class SysTiming
{
public:
    SysTiming();
    SysTiming(uint32_t vilimit);

    ~SysTiming();

    void startTimers();

    uint64_t doVILimit();
    void doFreqLimit();

    void setLimitMode(LimitMode mode)
    {
        _limitmode = mode;
        startTimers();
    }

private:
    uint64_t _framesElapsed;
    TimingMode _mode;
    LimitMode _limitmode;

    boost::chrono::high_resolution_clock::time_point _lastVITime;
    boost::chrono::high_resolution_clock::time_point _lastTick;
};
