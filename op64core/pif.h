#pragma once

#include <cstdint>
#include "inputtypes.h"
#include "eeprom.h"
#include "mempak.h"


class PIF
{
public:
    PIF(void);
    ~PIF(void);

    void initialize(void);
    void pifRead(void);
    void pifWrite(void);

private:
    void readController(int32_t controller, uint8_t* cmd);
    void controllerCommand(int32_t controller, uint8_t* cmd);

private:
    CONTROL _controllers[4];
    EEPROM* _eeprom;
    MemPak* _mempak;
};
