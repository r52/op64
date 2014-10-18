#pragma once

#include <cstdint>
#include <boost/chrono/chrono.hpp>

enum TimingMode
{
    TIMING_NTSC = 0,
    TIMING_PAL
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

private:
    uint64_t _framesElapsed;
    TimingMode _mode;

    boost::chrono::high_resolution_clock::time_point _lastTime;
};
