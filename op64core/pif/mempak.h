#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class Rom;

class MemPak
{
public:
    MemPak() = default;
    ~MemPak();

    static uint8_t calculateCRC(uint8_t* src);

    void read(Rom* rom, int control, int address, uint8_t* buf);
    void write(Rom* rom, int control, int address, uint8_t* buf);

private:
    void loadMempak(Rom* rom);

private:
    uint8_t _mempaks[4][0x8000];
    boost::filesystem::fstream _mempakfile;
};
