#include <cstdlib>

#include <boost/filesystem/fstream.hpp>

#include "rom.h"
#include "logger.h"
#include "util.h"

using namespace boost::filesystem;

static SystemType rom_country_code_to_system_type(uint16_t cc)
{
    switch (cc & 0xFF)
    {
        // PAL codes
    case 0x44:
    case 0x46:
    case 0x49:
    case 0x50:
    case 0x53:
    case 0x55:
    case 0x58:
    case 0x59:
        return SYSTEM_PAL;

        // NTSC codes
    case 0x37:
    case 0x41:
    case 0x45:
    case 0x4a:
    default: // Fallback for unknown codes
        return SYSTEM_NTSC;
    }
}

static uint32_t rom_system_type_to_ai_dac_rate(SystemType system_type)
{
    switch (system_type)
    {
    case SYSTEM_PAL:
        return 49656530;
    case SYSTEM_MPAL:
        return 48628316;
    case SYSTEM_NTSC:
    default:
        return 48681812;
    }
}

Rom::Rom(void) :
_image(nullptr),
_imagesize(0),
_imagetype(0),
_systemtype(SYSTEM_NTSC),
_savetype(SAVETYPE_AUTO)
{
}

Rom::~Rom(void)
{
    if (nullptr != _image)
    {
        delete[] _image;
        _image = nullptr;
    }

    Bus::rom = nullptr;
}

bool Rom::isValidRom(const uint8_t* image)
{
    // .z64
    if ((image[0] == 0x80) && (image[1] == 0x37) && (image[2] == 0x12) && (image[3] == 0x40))
    {
        return true;
    }
    // .v64
    else if ((image[0] == 0x37) && (image[1] == 0x80) && (image[2] == 0x40) && (image[3] == 0x12))
    {
        return true;
    }
    // .n64
    else if ((image[0] == 0x40) && (image[1] == 0x12) && (image[2] == 0x37) && (image[3] == 0x80))
    {
        return true;
    }

    return false;
}

void Rom::swapRom(uint8_t* rom, uint_fast32_t size)
{
    switch (rom[0])
    {
        // .v64
    case 0x37:
        _imagetype = V64IMAGE;
        vec_for(uint32_t i = 0; i < size; i += 2)
        {
            std::swap(rom[i], rom[i + 1]);
        }
        break;
        // .n64
    case 0x40:
        _imagetype = N64IMAGE;
        vec_for(uint32_t i = 0; i < size; i += 4)
        {
            std::swap(rom[i], rom[i + 3]);
            std::swap(rom[i + 1], rom[i + 2]);
        }
        break;
    default:
        _imagetype = Z64IMAGE;
        break;
    }
}


bool Rom::loadRom(const char* name)
{
    if (nullptr != _image)
    {
        // just fail
        return false;
    }

    _filename = boost::filesystem::path(name);
    if (!exists(_filename) || !is_regular_file(_filename))
    {
        // file doesn't exist or is bad
        return false;
    }

    ifstream file(_filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open() && file.good())
    {
        std::streampos size = file.tellg();

        if (size < 4096)
        {
            return false;
        }


        uint8_t* image = new uint8_t[static_cast<unsigned int>(size)];
        if (nullptr == image)
        {
            // Couldn't allocate
            return false;
        }

        file.seekg(0, std::ios::beg);
        file.read((char*)image, size);
        file.close();

        if (!isValidRom(image))
        {
            delete[] image;
            return false;
        }

        _image = image;
        _imagesize = static_cast<unsigned int>(size);

        swapRom(_image, _imagesize);

        // TODO future: complete rom info stuffs here
        memcpy(&_header, _image, sizeof(rom_header));

        _systemtype = rom_country_code_to_system_type(_header.Country_code);
        _vilimit = (_systemtype == SYSTEM_NTSC) ? 60 : 50;
        _aidacrate = rom_system_type_to_ai_dac_rate(_systemtype);
        // TODO future count per op

        // Swap it again because little endian cpu
        uint32_t* roml = (uint32_t*)_image;
        vec_for (uint32_t i = 0; i < (_imagesize / 4); i++)
        {
            roml[i] = byteswap_u32(roml[i]);
        }

        Bus::rom_image = _image;
        calculateCIC();

        return true;
    }

    return false;
}

void Rom::calculateCIC(void)
{
    if (nullptr == _image)
    {
        _cicchip = CIC_UNKNOWN;
        return;
    }

    int64_t CRC = 0;

    for (int32_t i = 0x40; i < 0x1000; i += 4)
    {
        CRC += *(uint32_t*)(_image + i);
    }

    switch (CRC) {
    case 0x000000D0027FDF31:
    case 0x000000CFFB631223:
        _cicchip = CIC_NUS_6101;
        break;
    case 0x000000D057C85244:
        _cicchip = CIC_NUS_6102;
        break;
    case 0x000000D6497E414B:
        _cicchip = CIC_NUS_6103;
        break;
    case 0x0000011A49F60E96:
        _cicchip = CIC_NUS_6105;
        break;
    case 0x000000D6D5BE5580:
        _cicchip = CIC_NUS_6106;
        break;
    default:
        _cicchip = CIC_UNKNOWN;
        break;
    }
}

