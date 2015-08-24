#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class Rom;

class EEPROM
{
public:
    EEPROM() = default;
    ~EEPROM();

public:
    void eepromCommand(Rom* rom, uint8_t* command);

private:
    void loadEEPROM(Rom* rom);
    void read(Rom* rom, uint8_t* buf, int line);
    void write(Rom* rom, uint8_t* buf, int line);

private:
    uint8_t _eeprom[0x800] = { 0xff };
    boost::filesystem::fstream _eepfile;
};
