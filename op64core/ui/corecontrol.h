#pragma once

#include <cstdint>
#include <atomic>

class CoreControl
{
public:
    static std::atomic<double> fps;
};
