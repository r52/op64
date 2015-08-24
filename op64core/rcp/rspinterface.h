#pragma once

#include "rcpinterface.h"
#include "registerinterface.h"

#define SP_MEM_SIZE 0x2000

class RSPInterface : public RCPInterface, public RegisterInterface
{
public:
    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

    void prepareRSP(Bus* bus);

private:
    OPStatus readMem(Bus* bus, uint32_t address, uint32_t* data);
    OPStatus readReg(Bus* bus, uint32_t address, uint32_t* data);
    OPStatus readStat(Bus* bus, uint32_t address, uint32_t* data);

    OPStatus writeMem(Bus* bus, uint32_t address, uint32_t data, uint32_t mask);
    OPStatus writeReg(Bus* bus, uint32_t address, uint32_t data, uint32_t mask);
    OPStatus writeStat(Bus* bus, uint32_t address, uint32_t data, uint32_t mask);

    void updateReg(Bus* bus, uint32_t w);

    void DMARead();
    void DMAWrite();

public:
    uint32_t mem[SP_MEM_SIZE / 4];    // allocate exactly 2048 * (4 byte uint32) = 8192 bytes (8KB) of SP MEM
    uint32_t* const dmem = mem;
    uint32_t* const imem = dmem + ((SP_MEM_SIZE / 2) / 4);     // IMEM is 4KB (0x400 = 1024 4 byte ints) into the memory
    
    uint32_t stat[SP2_NUM_REGS];
};
