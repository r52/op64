#include "systiming.h"
#include <boost/thread/thread.hpp>

using namespace boost::chrono;

typedef duration<int_least64_t, boost::ratio<1, 60>> NTSCFrame;
typedef duration<int_least64_t, boost::ratio<1, 50>> PALFrame;

SysTiming::SysTiming() :
_framesElapsed(0),
_mode(TIMING_NTSC)
{
}

SysTiming::SysTiming(uint32_t vilimit) :
_framesElapsed(0)
{
    _mode = (vilimit == 60) ? TIMING_NTSC : TIMING_PAL;
}


SysTiming::~SysTiming()
{
}


uint64_t SysTiming::doVILimit()
{
    ++_framesElapsed;

    auto modeledTime = _lastTime + NTSCFrame(_framesElapsed);

    if (_mode == TIMING_PAL)
    {
        modeledTime = _lastTime + PALFrame(_framesElapsed);
    }

    auto curtime = high_resolution_clock::now();

    if (curtime < modeledTime)
    {
        boost::this_thread::sleep_for(modeledTime - curtime);
    }

    if (curtime - _lastTime > milliseconds(1000))
    {
        uint64_t frames = _framesElapsed;
        _framesElapsed = 0;
        _lastTime = high_resolution_clock::now();

        return frames;
    }

    return 0;
}

void SysTiming::startTimers()
{
    _lastTime = high_resolution_clock::now();
}
