#pragma once

#include "cheat.h"

enum CheatEntry
{
    ENTRY_BOOT = 0,
    ENTRY_VI
};

class CheatEngine
{
public:
    static CheatCodeList toCheatCodeList(const std::string& strcodelist);

    CheatEngine();
    ~CheatEngine();

    void addRomHacks(CheatList romhacks);
    void applyCheats(CheatEntry entry);

private:
    static CheatCodeList processCodeList(CheatCodeList& rawlist);

    bool execute_cheat(uint32_t address, uint16_t value, int32_t* old_value);

private:
    CheatList _romhacks;
    bool _gsButtonPressed = false;
};
