#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <logger.h>
#include <compiler.h>

#include "romdb.h"

#include <cheat/cheatengine.h>

#define ROMDB_FILE "romdb.ini"

RomDB::RomDB(void)
{
    boost::filesystem::path dbpath(ROMDB_FILE);

    if (exists(dbpath))
    {
        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(ROMDB_FILE, pt);
        uint32_t entries = 0;
        std::string save;
        std::string crc;
        std::string refmd5;
        std::string cheats;

        for (auto& section : pt)
        {
            ++entries;
            RomSettings setting;
            uint32_t numhacks = 0;
            for (auto& key : section.second)
            {
                if (key.first == "GoodName")
                {
                    setting.goodname = key.second.get_value<std::string>();
                }
                else if (key.first == "SaveType")
                {
                    save = key.second.get_value<std::string>();
                    if (save == "Eeprom 4KB")
                    {
                        setting.savetype = SAVETYPE_EEPROM_4KB;
                    }
                    else if (save == "Eeprom 16KB")
                    {
                        setting.savetype = SAVETYPE_EEPROM_16KB;
                    }
                    else if (save == "SRAM")
                    {
                        setting.savetype = SAVETYPE_SRAM;
                    }
                    else if (save == "Flash RAM")
                    {
                        setting.savetype = SAVETYPE_FLASH_RAM;
                    }
                    else if (save == "Controller Pack")
                    {
                        setting.savetype = CONTROLLER_PACK;
                    }
                    else if (save == "None")
                    {
                        setting.savetype = SAVETYPE_NONE;
                    }
                    else
                    {
                        LOG_WARNING("RomDB: invalid save type value in entry %s", section.first.c_str());
                    }
                }
                else if (key.first == "Status")
                {
                    setting.status = key.second.get_value<uint32_t>();
                }
                else if (key.first == "CRC")
                {
                    crc = key.second.get_value<std::string>();
                    if (_s_sscanf(crc.c_str(), "%X %X", &setting.crc1, &setting.crc2) != 2)
                    {
                        LOG_WARNING("RomDB: invalid crc value in entry %s", section.first.c_str());
                    }
                }
                else if (key.first == "RefMD5")
                {
                    refmd5 = key.second.get_value<std::string>();
                    auto search = _db.find(refmd5);
                    if (search != _db.end())
                    {
                        // copy it
                        setting = search->second;
                    }
                    else
                    {
                        LOG_WARNING("RomDB: referenced md5 value not found in entry %s", section.first.c_str());
                    }
                }
                else if (key.first == "CountPerOp")
                {
                    setting.countperop = key.second.get_value<uint32_t>();
                    if (setting.countperop < 1 || setting.countperop > 4)
                    {
                        setting.countperop = 2;
                        LOG_WARNING("RomDB: invalid count per op value in entry %s", section.first.c_str());
                    }
                }
                else if (key.first.find("Cheat") != std::string::npos)
                {
                    Cheat newhack;

                    char namebuf[100];
                    _s_snprintf(namebuf, 100, "Hack%d", ++numhacks);
                    newhack.name = std::string(namebuf);

                    cheats = key.second.get_value<std::string>();
                    newhack.codes = CheatEngine::toCheatCodeList(cheats);
                    newhack.enabled = newhack.was_enabled = true;

                    setting.romhacks.push_back(newhack);
                }
                else
                {
                    LOG_WARNING("RomDB: unrecognized key %s in entry %s", key.first.c_str(), section.first.c_str());
                }
            }

            _db[section.first] = setting;
        }

        LOG_INFO("RomDB: %d entries loaded", entries);
    }
    else
    {
        LOG_WARNING("RomDB: rom database file not found");
    }
}

bool RomDB::get(std::string rom_md5, RomSettings& copy)
{
    auto search = _db.find(rom_md5);
    if (search != _db.end())
    {
        copy = search->second;
        return true; 
    }

    return false;
}

RomDB::~RomDB(void)
{

}
