#include "corecontrol.h"

std::atomic<double> CoreControl::fps = 0.0;

// Not exposed right now
uint32_t CoreControl::VIRefreshRate = 1500;
