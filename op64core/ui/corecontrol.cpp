#include "corecontrol.h"

std::atomic<bool> CoreControl::stop{ true };
std::atomic<bool> CoreControl::doHardReset{ false };
std::atomic<bool> CoreControl::limitVI{ true };

std::atomic<double> CoreControl::fps = 0.0;

// Not exposed right now
uint32_t CoreControl::VIRefreshRate = 1500;
