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
    FlashRam() = default;
    ~FlashRam();

    void dmaToFlash(Bus* bus, uint8_t* src, int32_t offset, int32_t len);
    void dmaFromFlash(Bus* bus, uint8_t* dest, int32_t offset, int32_t len);

    // Interface
    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

private:
    uint32_t readFlashStatus(void);
    void writeFlashCommand(Bus* bus, uint32_t command);
    void loadFlashRam(Bus* bus);

private:
    FlashRamMode _mode = NOPES_MODE;
    uint64_t _status = 0;
    uint8_t* _writepointer = nullptr;
    uint32_t _offset = 0;
    boost::filesystem::fstream _flashramfile;
};
