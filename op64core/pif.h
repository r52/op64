#pragma once

#include "rcpinterface.h"
#include "eeprom.h"
#include "mempak.h"

#define PIF_RAM_SIZE 0x40

class PIF : public RCPInterface
{
public:
    PIF(void);
    ~PIF(void);

    void initialize(void);
    void uninitialize(void);
    void pifRead(void);
    void pifWrite(void);

    // Interface
    virtual OPStatus read(uint32_t address, uint32_t* data) override;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) override;

private:
    void readController(int32_t controller, uint8_t* cmd);
    void controllerCommand(int32_t controller, uint8_t* cmd);

public:
    uint8_t ram[PIF_RAM_SIZE];

private:
    EEPROM* _eeprom;
    MemPak* _mempak;
};
