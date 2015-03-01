#include <cstdlib>

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include "rom.h"
#include "logger.h"
#include "util.h"
#include "md5.h"
#include "romdb.h"

using namespace boost::filesystem;

static const char* systemTypeString[SYSTEM_NUM_TYPES] =
{
    "NTSC",
    "PAL",
    "MPAL"
};

static const char* saveTypeString[SAVETYPE_NUM_TYPES] =
{
    "Auto",
    "EEPROM 4KB",
    "EEPROM 16KB",
    "SRAM",
    "Flash RAM",
    "Controller Pak",
    "None"
};

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
    if (nullptr == image)
    {
        LOG_ERROR("ROM: null image");
        return false;
    }

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


bool Rom::loadRom(const char* name, Rom*& outRom)
{
    Rom* rom = new Rom();

    if (nullptr != rom->_image)
    {
        // Sanity
        LOG_ERROR("ROM: unrecoverable error");
        delete rom;
        return false;
    }

    rom->_filename = boost::filesystem::path(name);
    if (!exists(rom->_filename) || !is_regular_file(rom->_filename))
    {
        // file doesn't exist or is bad
        LOG_ERROR("ROM: file %s not found or is an invalid file", name);
        delete rom;
        return false;
    }

    ifstream file(rom->_filename, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open() && file.good())
    {
        LOG_INFO("ROM: Loading %s", name);

        std::streampos size = file.tellg();

        if (size < 4096)
        {
            LOG_ERROR("ROM: bad ROM size: %u", size);
            delete rom;
            return false;
        }

        uint8_t* image = new uint8_t[static_cast<unsigned int>(size)];
        if (nullptr == image)
        {
            LOG_ERROR("ROM: error allocating %u bytes for ROM!", size);
            delete rom;
            return false;
        }

        file.seekg(0, std::ios::beg);
        file.read((char*)image, size);
        file.close();

        if (!isValidRom(image))
        {
            LOG_ERROR("ROM: invalid N64 ROM");
            delete[] image;
            delete rom;
            return false;
        }

        rom->_image = image; image = nullptr;
        rom->_imagesize = static_cast<unsigned int>(size);

        rom->swapRom(rom->_image, rom->_imagesize);

        memcpy(&rom->_header, rom->_image, sizeof(rom_header));

        // Calculate md5 for rom db search
        MD5 md = MD5();
        md.update(rom->_image, rom->_imagesize);
        md.finalize();
        rom->_md5 = boost::to_upper_copy(md.hexdigest());

        rom->_systemtype = rom_country_code_to_system_type(rom->_header.Country_code);
        rom->_vilimit = (rom->_systemtype == SYSTEM_NTSC) ? 60 : 50;
        rom->_aidacrate = rom_system_type_to_ai_dac_rate(rom->_systemtype);

        RomSettings settings;
        if (RomDB::getInstance().get(rom->_md5, settings))
        {
            rom->_count_per_op = settings.countperop;
            rom->_savetype = settings.savetype;
            rom->_goodname = settings.goodname;
            rom->_romhacks = settings.romhacks;
        }

        LOG_VERBOSE("GoodName: %s", rom->_goodname.c_str());
        LOG_VERBOSE("Name: %s", rom->_header.Name);
        LOG_VERBOSE("CRC: %X %X", rom->_header.CRC1, rom->_header.CRC2);
        LOG_VERBOSE("System Type: %s", systemTypeString[rom->_systemtype]);
        LOG_VERBOSE("Save Type: %s", saveTypeString[rom->_savetype]);
        LOG_VERBOSE("Rom Size: %d Megabits", rom->_imagesize / 1024 / 1024 * 8);
        LOG_VERBOSE("ClockRate: %X", byteswap_u32(rom->_header.ClockRate));
        LOG_VERBOSE("Release Code: %X", byteswap_u32(rom->_header.Release));
        LOG_VERBOSE("Manufacturer ID: %X", byteswap_u32(rom->_header.Manufacturer_ID));
        LOG_VERBOSE("Cartridge ID: %X", rom->_header.Cartridge_ID);
        LOG_VERBOSE("Country Code: %X", rom->_header.Country_code);
        LOG_VERBOSE("PC: 0x%X", byteswap_u32(rom->_header.PC));

        LOG_DEBUG("ROM: %d ROM hacks enabled", rom->_romhacks.size());

        rom->setGameHacks(rom->_header.Cartridge_ID);

        // Swap it again because little endian cpu
        uint32_t* roml = (uint32_t*)rom->_image;
        vec_for(uint32_t i = 0; i < (rom->_imagesize / 4); i++)
        {
            roml[i] = byteswap_u32(roml[i]);
        }

        rom->calculateCIC();

        outRom = rom; rom = nullptr;
        LOG_INFO("ROM: ROM loaded");

        return true;
    }

    delete rom;
    LOG_ERROR("ROM: bad ROM file");

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

void Rom::setGameHacks(uint16_t cartid)
{
    switch (cartid)
    {
    case 0x4547:
        _gamehacks |= GAME_HACK_GOLDENEYE;
        break;
    default:
        break;
    }
}

OPStatus Rom::read(uint32_t address, uint32_t* data)
{
    uint32_t addr = ROM_ADDRESS(address);

    if (_rom_lastwrite != 0)
    {
        *data = _rom_lastwrite;
        _rom_lastwrite = 0;
    }
    else
    {
        *data = *(uint32_t*)(_image + addr);
    }

    return OP_OK;
}

OPStatus Rom::write(uint32_t address, uint32_t data, uint32_t mask)
{
    _rom_lastwrite = data & mask;

    return OP_OK;
}

