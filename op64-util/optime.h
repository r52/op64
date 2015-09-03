#pragma once

#include <ctime>
#include "oppreproc.h"

namespace op
{
    OP_API tm localtime(const std::time_t& time);
}
