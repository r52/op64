#pragma once

#include <map>

#include "rom/rom.h"
#include "cheat/cheat.h"

struct RomSettings
{
    std::string goodname;
    uint32_t crc1;
    uint32_t crc2;
    SaveType savetype;
    uint32_t status;
    uint8_t countperop = 2;
    CheatList romhacks;
};

class RomDB
{
    typedef std::map<std::string, RomSettings> RomDatabase;
public:
    static RomDB& getInstance()
    {
        static RomDB instance;
        return instance;
    }

    bool get(std::string rom_md5, RomSettings& copy);

private:
    RomDB(void);
    ~RomDB(void);

    // Not implemented
    RomDB(const RomDB&);
    RomDB& operator=(const RomDB&);

private:
    RomDatabase _db;
};
