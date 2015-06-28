#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class MemPak
{
public:
    MemPak() = default;
    ~MemPak();

    static uint8_t calculateCRC(uint8_t* src);

    void read(int control, int address, uint8_t* buf);
    void write(int control, int address, uint8_t* buf);

private:
    void loadMempak(void);

private:
    uint8_t _mempaks[4][0x8000];
    boost::filesystem::fstream _mempakfile;
};
