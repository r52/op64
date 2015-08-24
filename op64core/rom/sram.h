#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class Bus;

class SRAM
{
public:
    SRAM(void) = default;
    ~SRAM(void);

    void dmaToSRAM(Bus* bus, uint8_t* src, int32_t offset, int32_t len);
    void dmaFromSRAM(Bus* bus, uint8_t* dest, int32_t offset, int32_t len);

private:
    void loadSRAM(Bus* bus);

private:
    boost::filesystem::fstream _sramfile;
};
