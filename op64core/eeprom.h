#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class EEPROM
{
public:
    EEPROM();
    ~EEPROM();

public:
    void eepromCommand(uint8_t* command);

private:
    void loadEEPROM(void);
    void read(uint8_t* buf, int line);
    void write(uint8_t* buf, int line);

private:
#ifdef HAS_CXX11_LIST_INST
    uint8_t _eeprom[0x800] = { 0xff };
#else
    uint8_t _eeprom[0x800];
#endif
    boost::filesystem::fstream _eepfile;
};
