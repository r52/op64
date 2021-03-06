#include <oplog.h>
#include <optime.h>

#include "eeprom.h"

#include <globalstrings.h>
#include <core/bus.h>
#include <rom/rom.h>
#include <ui/configstore.h>

static unsigned char byte2bcd(int n)
{
    n %= 100;
    return (unsigned char)((n / 10) << 4) | (n % 10);
}

void EEPROM::eepromCommand(Rom* rom, uint8_t* command)
{
    BOOST_LOG_NAMED_SCOPE(__STR__(__COUNTER__));

    if (rom->getSaveType() == SAVETYPE_AUTO)
    {
        rom->setSaveType(SAVETYPE_EEPROM_4KB);
    }

    switch (command[2])
    {
    case 0: // check
        if (rom->getSaveType() != SAVETYPE_EEPROM_4KB &&  rom->getSaveType() != SAVETYPE_EEPROM_16KB) {
            command[1] |= 0x80;
            break;
        }
        if (command[1] != 3)
        {
            command[1] |= 0x40;
            if ((command[1] & 3) > 0)
            {
                command[3] = 0;
            }
            if ((command[1] & 3) > 1)
            {
                command[4] = (rom->getSaveType() != SAVETYPE_EEPROM_16KB) ? 0x80 : 0xc0;
            }
            if ((command[1] & 3) > 2)
            {
                command[5] = 0;
            }
        }
        else
        {
            command[3] = 0;
            command[4] = (rom->getSaveType() != SAVETYPE_EEPROM_16KB) ? 0x80 : 0xc0;
            command[5] = 0;
        }
        break;
    case 4: // read
    {
        read(rom, &command[4], command[3]);
    }
        break;
    case 5: // write
    {
        write(rom, &command[4], command[3]);
    }
        break;
    case 6:
        // RTCstatus query
        command[3] = 0x00;
        command[4] = 0x10;
        command[5] = 0x00;
        break;
    case 7:
        // read RTC block
        switch (command[3]) // block number
        {
        case 0:
            command[4] = 0x00;
            command[5] = 0x02;
            command[12] = 0x00;
            break;
        case 1:
            // Can't use the macro here because c2360
            LOG_LEVEL(EEPROM, warning) << "RTC command: read block " << command[2];
            break;
        case 2:
            time_t curtime_time;
            time(&curtime_time);
            tm curtime = op::localtime(curtime_time);
            command[4] = byte2bcd(curtime.tm_sec);
            command[5] = byte2bcd(curtime.tm_min);
            command[6] = 0x80 + byte2bcd(curtime.tm_hour);
            command[7] = byte2bcd(curtime.tm_mday);
            command[8] = byte2bcd(curtime.tm_wday);
            command[9] = byte2bcd(curtime.tm_mon + 1);
            command[10] = byte2bcd(curtime.tm_year);
            command[11] = byte2bcd(curtime.tm_year / 100);
            command[12] = 0x00;	// status
            break;
        }
        break;
    case 8:
    {
        // write RTC block
        LOG_WARNING(EEPROM) << "RTC write: " << command[2] << " not yet implemented";
    }
        break;
    default:
    {
        LOG_WARNING(EEPROM) << "Unknown command: " << std::hex << command[2];
    }
        break;
    }
}

void EEPROM::loadEEPROM(Rom* rom)
{
    using namespace boost::filesystem;

    path eeppath(ConfigStore::getInstance().getString(GlobalStrings::CFG_SECTION_CORE, "SavePath") + rom->getRomFilenameNoExtension() + ".eep");

    if (!exists(eeppath.parent_path()))
    {
        create_directories(eeppath.parent_path());
    }

    if (!exists(eeppath))
    {
        _eepfile.open(eeppath, std::ios::out);
        _eepfile.write((char*)_eeprom, sizeof(_eeprom));
        _eepfile.close();
    }

    _eepfile.open(eeppath, std::ios::in | std::ios::out | std::ios::binary);
    _eepfile.seekg(0, std::ios::beg);
    _eepfile.read((char*)_eeprom, sizeof(_eeprom));
}

void EEPROM::read(Rom* rom, uint8_t* buf, int line)
{
    if (!_eepfile.is_open())
    {
        loadEEPROM(rom);
    }

    for (int i = 0; i < 8; i++)
    {
        buf[i] = _eeprom[line * 8 + i];
    }
}

void EEPROM::write(Rom* rom, uint8_t* buf, int line)
{
    if (!_eepfile.is_open())
    {
        loadEEPROM(rom);
    }

    for (int i = 0; i < 8; i++)
    { 
        _eeprom[line * 8 + i] = buf[i];
    }

    _eepfile.seekp(line*8, std::ios::beg);
    _eepfile.write((char*)buf, 8);
    _eepfile.flush();
}

EEPROM::~EEPROM()
{
    if (_eepfile.is_open())
    {
        _eepfile.close();
    }
}
