#pragma once

#include <memory>
#include <rcp/rcpinterface.h>

#include <pif/mempak.h>
#include <pif/eeprom.h>

#define PIF_RAM_SIZE 0x40

class Bus;

class PIF : public RCPInterface
{
public:
    PIF(void) = default;
    ~PIF(void);

    bool initialize(void);
    void uninitialize(void);
    void pifRead(Bus* bus);
    void pifWrite(Bus* bus);

    // Interface
    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

private:
    void readController(Bus* bus, int32_t controller, uint8_t* cmd);
    void controllerCommand(Bus* bus, int32_t controller, uint8_t* cmd);

public:
    uint8_t ram[PIF_RAM_SIZE];

private:
    std::unique_ptr<EEPROM> _eeprom;
    std::unique_ptr<MemPak> _mempak;
};
