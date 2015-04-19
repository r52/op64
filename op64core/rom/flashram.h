#pragma once

#include <cstdint>

#include <boost/filesystem/fstream.hpp>

#include <rcp/rcpinterface.h>

class FlashRam : public RCPInterface
{
    enum FlashRamMode
    {
        NOPES_MODE = 0,
        ERASE_MODE,
        WRITE_MODE,
        READ_MODE,
        STATUS_MODE
    };

public:
    FlashRam();
    ~FlashRam();

    void dmaToFlash(uint8_t* src, int32_t offset, int32_t len);
    void dmaFromFlash(uint8_t* dest, int32_t offset, int32_t len);
    uint32_t readFlashStatus(void);
    void writeFlashCommand(uint32_t command);

    // Interface
    virtual OPStatus read(uint32_t address, uint32_t* data) override;
    virtual OPStatus write(uint32_t address, uint32_t data, uint32_t mask) override;

private:
    void loadFlashRam(void);

private:
    FlashRamMode _mode;
    uint64_t _status;
    uint8_t* _writepointer;
    uint32_t _offset;
    boost::filesystem::fstream _flashramfile;
};
