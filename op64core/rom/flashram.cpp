#include <oplog.h>

#include "flashram.h"

#include <core/bus.h>
#include <ui/configstore.h>
#include <rom/rom.h>


static char emptyBlock[128] = { 0xffu };

FlashRam::FlashRam() :
_mode(NOPES_MODE),
_status(0),
_writepointer(nullptr),
_offset(0)
{
}

FlashRam::~FlashRam()
{
    if (_flashramfile.is_open())
    {
        _flashramfile.close();
    }
}

void FlashRam::loadFlashRam(void)
{
    using namespace boost::filesystem;

    path flashpath(ConfigStore::getInstance().getString(CFG_SECTION_CORE, "SavePath") + Bus::rom->getRomFilenameNoExtension() + ".fla");

    if (!exists(flashpath.parent_path()))
    {
        create_directories(flashpath.parent_path());
    }

    if (!exists(flashpath))
    {
        _flashramfile.open(flashpath, std::ios::out);
        _flashramfile.seekp(0x1ffff);
        _flashramfile.put(0xffu);
        _flashramfile.close();
    }

    _flashramfile.open(flashpath, std::ios::in | std::ios::out | std::ios::binary);
    _flashramfile.seekg(0, std::ios::beg);
    _flashramfile.seekp(0, std::ios::beg);
}

void FlashRam::dmaFromFlash(uint8_t* dest, int32_t offset, int32_t len)
{
    switch (_mode)
    {
    case FlashRam::STATUS_MODE:
        *((uint32_t*)(dest)) = (uint32_t)((_status >> 32) & 0xFFFFFFFF);
        *((uint32_t*)(dest)+1) = (uint32_t)(_status & 0xFFFFFFFF);
        break;
    case FlashRam::READ_MODE:
        if (!_flashramfile.is_open())
        {
            loadFlashRam();
        }

        _flashramfile.seekg(offset*2, std::ios::beg);
        _flashramfile.read((char*)dest, len);
        break;
    default:
        LOG_ERROR(FlashRam) << "Unknown DMA FlashRAM read mode: " << std::hex << _mode;
        Bus::stop = true;
        break;
    }
}

void FlashRam::dmaToFlash(uint8_t* src, int32_t offset, int32_t len)
{
    switch (_mode)
    {
    case FlashRam::WRITE_MODE:
        _writepointer = src;
        break;
    default:
        LOG_ERROR(FlashRam) << "Unknown DMA FlashRAM write mode: " << std::hex << _mode;
        Bus::stop = true;
        break;
    }
}

uint32_t FlashRam::readFlashStatus(void)
{
    return (uint32_t)(_status >> 32);
}

void FlashRam::writeFlashCommand(uint32_t command)
{
    switch (command & 0xff000000)
    {
    case 0x4b000000:
        _offset = (command & 0xffff) * 128;
        break;
    case 0x78000000:
        _mode = ERASE_MODE;
        _status = 0x1111800800c20000;
        break;
    case 0xa5000000:
        _offset = (command & 0xffff) * 128;
        _status = 0x1111800400c20000;
        break;
    case 0xb4000000:
        _mode = WRITE_MODE;
        break;
    case 0xd2000000:  // execute
        switch (_mode)
        {
        case NOPES_MODE:
        case READ_MODE:
            break;
        case ERASE_MODE:
        {
            if (!_flashramfile.is_open())
            {
                loadFlashRam();
            }

            _flashramfile.seekp(_offset);
            _flashramfile.write(emptyBlock, 128);
        }
            break;
        case WRITE_MODE:
        {
            if (!_flashramfile.is_open())
            {
                loadFlashRam();
            }

            _flashramfile.seekp(_offset);
            _flashramfile.write((char*)_writepointer, 128);
        }
            break;
        case STATUS_MODE:
            break;
        default:
            LOG_ERROR(FlashRam) << "Unknown flashram command with mode: " << std::hex << _mode;
            Bus::stop = true;
            break;
        }
        _mode = NOPES_MODE;
        break;
    case 0xe1000000:
        _mode = STATUS_MODE;
        _status = 0x1111800100c20000;
        break;
    case 0xf0000000:
        _mode = READ_MODE;
        _status = 0x11118004f0000000;
        break;
    default:
        LOG_WARNING(FlashRam) << "Unknown flashram command: %x" << std::hex << command;
        break;
    }
}

OPStatus FlashRam::write(uint32_t address, uint32_t data, uint32_t mask)
{
    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        writeFlashCommand(data & mask);
    }
    else
    {
        LOG_WARNING(FlashRam) << "Writing flashram command with non-flashram save type";
    }

    return OP_OK;
}

OPStatus FlashRam::read(uint32_t address, uint32_t* data)
{
    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        *data = readFlashStatus();
    }
    else
    {
        LOG_WARNING(FlashRam) << "Reading flashram command with non-flashram save type";
    }

    return OP_OK;
}
