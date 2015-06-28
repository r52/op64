#pragma once

#include <cstdint>
#include <boost/filesystem/fstream.hpp>

class SRAM
{
public:
    SRAM(void) = default;
    ~SRAM(void);

    void dmaToSRAM(uint8_t* src, int32_t offset, int32_t len);
    void dmaFromSRAM(uint8_t* dest, int32_t offset, int32_t len);

private:
    void loadSRAM(void);

private:
    boost::filesystem::fstream _sramfile;
};
