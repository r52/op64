#pragma once

#include <cstdint>
#include <boost/filesystem.hpp>

#include <cheat/cheat.h>
#include <rcp/rcpinterface.h>

enum SystemType
{
    SYSTEM_NTSC = 0,
    SYSTEM_PAL,
    SYSTEM_MPAL,
    SYSTEM_NUM_TYPES
};

enum CIC
{
    CIC_UNKNOWN = 0,
    CIC_NUS_6101 = 1,
    CIC_NUS_6102,
    CIC_NUS_6103,
    CIC_NUS_6104,   // doesnt exist
    CIC_NUS_6105,
    CIC_NUS_6106,
    CIC_NUM_TYPES
};

enum ImageType
{
    Z64IMAGE,
    V64IMAGE,
    N64IMAGE
};

enum SaveType
{
    SAVETYPE_AUTO,
    SAVETYPE_EEPROM_4KB,
    SAVETYPE_EEPROM_16KB,
    SAVETYPE_SRAM,
    SAVETYPE_FLASH_RAM,
    CONTROLLER_PACK,
    SAVETYPE_NONE,
    SAVETYPE_NUM_TYPES
};

enum GameHacks
{
    GAME_HACK_NONE = 0,
    GAME_HACK_GOLDENEYE = 1
};

#define COUNT_PER_OP_DEFAULT 2

typedef struct
{
    uint8_t init_PI_BSB_DOM1_LAT_REG;  /* 0x00 */
    uint8_t init_PI_BSB_DOM1_PGS_REG;  /* 0x01 */
    uint8_t init_PI_BSB_DOM1_PWD_REG;  /* 0x02 */
    uint8_t init_PI_BSB_DOM1_PGS_REG2; /* 0x03 */
    uint32_t ClockRate;                  /* 0x04 */
    uint32_t PC;                         /* 0x08 */
    uint32_t Release;                    /* 0x0C */
    uint32_t CRC1;                       /* 0x10 */
    uint32_t CRC2;                       /* 0x14 */
    uint32_t Unknown[2];                 /* 0x18 */
    uint8_t Name[20];                  /* 0x20 */
    uint32_t unknown;                    /* 0x34 */
    uint32_t Manufacturer_ID;            /* 0x38 */
    uint16_t Cartridge_ID;             /* 0x3C - Game serial number  */
    uint16_t Country_code;             /* 0x3E */
} rom_header;

class Rom : public RCPInterface
{
public:
    ~Rom(void);

    static bool loadRom(const char* name, Rom*& outRom);

    virtual OPStatus read(Bus* bus, uint32_t address, uint32_t* data) override;
    virtual OPStatus write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask) override;

    inline uint8_t* getImage(void)
    {
        return _image;
    }

    inline uint8_t getSaveType(void)
    {
        return _savetype;
    }

    inline void setSaveType(SaveType type)
    {
        _savetype = type;
    }

    inline SystemType getSystemType(void)
    {
        return _systemtype;
    }

    inline rom_header* getHeader(void)
    {
        return &_header;
    }

    inline uint8_t getImageType(void)
    {
        return _imagetype;
    }

    inline uint32_t getSize(void)
    {
        return _imagesize;
    }

    inline uint32_t getViLimit(void)
    {
        return _vilimit;
    }

    inline uint32_t getAiDACRate(void)
    {
        return _aidacrate;
    }

    inline uint8_t getCountPerOp(void)
    {
        return _count_per_op;
    }

    inline uint32_t getCICChip(void)
    {
        return _cicchip;
    }

    inline uint32_t getGameHacks(void)
    {
        return _gamehacks;
    }

    inline std::string getRomFilename(void)
    {
        return _filename.filename().string();
    }

    inline std::string getRomFilenameNoExtension(void)
    {
        return _filename.stem().string();
    }

    inline std::string getRomExtension(void)
    {
        return _filename.extension().string();
    }

    inline CheatList& getRomHacks(void)
    {
        return _romhacks;
    }

private:
    Rom(void) = default;
    static bool isValidRom(const uint8_t* image);
    void swapRom(uint8_t* rom, uint_fast32_t size);
    void calculateCIC(void);
    void setGameHacks(uint16_t cartid);

private:
    uint8_t* _image = nullptr;
    uint_fast32_t _imagesize = 0;
    uint_fast8_t _imagetype = 0;
    uint_fast8_t _savetype = SAVETYPE_AUTO;
    SystemType _systemtype = SYSTEM_NTSC;
    uint32_t _cicchip = CIC_UNKNOWN;
    uint32_t _vilimit = 0;
    uint32_t _aidacrate = 0;
    uint8_t _count_per_op = COUNT_PER_OP_DEFAULT;
    uint32_t _gamehacks = GAME_HACK_NONE;

    std::string _md5;
    std::string _goodname;
    rom_header _header;
    boost::filesystem::path _filename;

    uint32_t _rom_lastwrite = 0;

    // Rom hacks that cannot be changed by the user
    CheatList _romhacks;

    // TODO: Actual cheats
    //CheatList _cheats;
};
