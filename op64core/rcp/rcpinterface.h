#pragma once

#include <op64.h>

// Optional I/O mode can be implemented by interfaces 
enum RCPIOMode
{
    RCP_IO_REG = 0,
    RCP_IO_MEM,
    RCP_IO_STAT
};

class RCPInterface
{
public:
    virtual ~RCPInterface() {}
    virtual OPStatus read(uint32_t address, uint32_t* data) = 0;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) = 0;

    inline void setIOMode(RCPIOMode mode)
    {
        _iomode = mode;
    }

protected:
    RCPIOMode _iomode = RCP_IO_REG;
};
