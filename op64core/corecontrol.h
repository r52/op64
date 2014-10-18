#pragma once

#include <cstdint>
#include <functional>

class CoreControl
{
public:
    static std::function<void(uint64_t)> displayVI;
};
