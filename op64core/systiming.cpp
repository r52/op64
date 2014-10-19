#include "systiming.h"
#include "bus.h"
#include <boost/thread/thread.hpp>


using namespace boost::chrono;

template <int_least64_t framerate>
struct GameClock
{
    typedef int_least64_t rep;
    typedef boost::chrono::duration<rep, boost::ratio<1, framerate>> duration;
};

typedef GameClock<60> NTSCClock;
typedef GameClock<50> PALClock;

template <int_least64_t speed>
struct CPUClock
{
    typedef typename boost::ratio_multiply<boost::ratio<speed>, boost::mega>::type frequency;  // Mhz
    typedef typename boost::ratio_divide<boost::ratio<4>, frequency>::type period;
    typedef int_least64_t rep;
    typedef boost::chrono::duration<rep, period> duration;

    static uint64_t tick;

    static duration now()
    {
        return duration(++tick);
    }

    static void reset()
    {
        tick = 0;
    }
};

typedef CPUClock<375> R4300Clock;
uint64_t R4300Clock::tick = 0;


SysTiming::SysTiming() :
_framesElapsed(0),
_mode(TIMING_NTSC),
_limitmode(LIMIT_BY_VI)
{
}

SysTiming::SysTiming(uint32_t vilimit) :
_framesElapsed(0),
_limitmode(LIMIT_BY_VI)
{
    _mode = (vilimit == 60) ? TIMING_NTSC : TIMING_PAL;
}


SysTiming::~SysTiming()
{
}


uint64_t SysTiming::doVILimit()
{
    ++_framesElapsed;

    auto curtime = high_resolution_clock::now();

    if (Bus::limitVI && _limitmode == LIMIT_BY_VI)
    {
        auto modeledTime = _lastVITime + NTSCClock::duration(_framesElapsed);

        if (_mode == TIMING_PAL)
        {
            modeledTime = _lastVITime + PALClock::duration(_framesElapsed);
        }

        if (curtime < modeledTime)
        {
            boost::this_thread::sleep_for(modeledTime - curtime);
        }
    }

    if (curtime - _lastVITime > milliseconds(1000))
    {
        uint64_t frames = _framesElapsed;
        _framesElapsed = 0;
        _lastVITime = high_resolution_clock::now();

        return frames;
    }

    return 0;
}

void SysTiming::startTimers()
{
    _lastVITime = high_resolution_clock::now();
    _lastTick = high_resolution_clock::now();
    R4300Clock::reset();
}

void SysTiming::doFreqLimit()
{
    auto curtime = high_resolution_clock::now();

    R4300Clock::duration tick = R4300Clock::now();

    if (Bus::limitVI && _limitmode == LIMIT_BY_FREQ)
    {
        auto modeledTime = _lastTick + tick;

        if (curtime < modeledTime)
        {
            boost::this_thread::sleep_for(modeledTime - curtime);
        }
    }

    if (curtime - _lastTick > milliseconds(1000))
    {
        R4300Clock::reset();
        _lastTick = high_resolution_clock::now();
    }
}
