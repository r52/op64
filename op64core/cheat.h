#pragma once

#include <cstdint>
#include <string>
#include <vector>

// A single cheat code
struct CheatCode
{
    uint32_t address;
    int32_t value;
    int32_t old_value;
};

// List of individual cheat codes
typedef std::vector<CheatCode> CheatCodeList;

// One cheat, which contains one or more cheat codes
struct Cheat
{
    std::string name;
    bool enabled;
    bool was_enabled;
    CheatCodeList codes;
};

// List of cheats
typedef std::vector<Cheat> CheatList;
