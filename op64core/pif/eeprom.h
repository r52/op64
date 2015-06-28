#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class EEPROM
{
public:
    EEPROM() = default;
    ~EEPROM();

public:
    void eepromCommand(uint8_t* command);

private:
    void loadEEPROM(void);
    void read(uint8_t* buf, int line);
    void write(uint8_t* buf, int line);

private:
    uint8_t _eeprom[0x800] = { 0xff };
    boost::filesystem::fstream _eepfile;
};
