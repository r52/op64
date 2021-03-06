#include <thread>
#include <ratio>

#include "systiming.h"
#include "bus.h"
#include <ui/corecontrol.h>

using namespace std::chrono;
using namespace std::literals::chrono_literals;

template <int_least64_t framerate>
struct GameClock
{
    typedef int_least64_t rep;
    typedef duration<rep, std::ratio<1, framerate>> duration;
};

typedef GameClock<60> NTSCClock;
typedef GameClock<50> PALClock;

template <int_least64_t speed>
struct CPUClock
{
    typedef typename std::ratio_multiply<std::ratio<speed>, std::mega>::type frequency;  // Mhz
    typedef typename std::ratio_divide<std::ratio<4>, frequency>::type period;
    typedef int_least64_t rep;
    typedef duration<rep, period> duration;
};

typedef CPUClock<375> R4300Clock;


static nanoseconds getGameClockFrame(TimingMode mode, uint64_t frames)
{
    switch (mode)
    {
    case TIMING_NTSC:
    default:
        return duration_cast<nanoseconds>(NTSCClock::duration{ frames });
        break;
    case TIMING_PAL:
        return duration_cast<nanoseconds>(PALClock::duration{ frames });
        break;
    }
}

SysTiming::SysTiming(uint32_t vilimit) :
    _mode((vilimit == 60) ? TIMING_NTSC : TIMING_PAL)
{
}

void SysTiming::doVILimit()
{
    ++_framesElapsed;

    if (CoreControl::limitVI && (_limitmode == LIMIT_BY_VI))
    {
        auto modeledTime = _lastKeyVITime + getGameClockFrame(_mode, _framesElapsed);
        auto frameDelta = modeledTime - high_resolution_clock::now();

        if (frameDelta > 0ns)
        {
            std::this_thread::sleep_for(frameDelta);
        }
    }

    auto elapsedTime = high_resolution_clock::now() - _lastKeyVITime;
    if (elapsedTime >= 1s)
    {
        CoreControl::fps = (double)_framesElapsed / std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime).count();
        _framesElapsed = 0;
        _lastKeyVITime = high_resolution_clock::now();
    }
}

void SysTiming::startTimers()
{
    _lastKeyVITime = high_resolution_clock::now();
    _framesElapsed = 0;
}

