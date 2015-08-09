#include "sram.h"

#include <boost/filesystem.hpp>

#include <globalstrings.h>
#include <ui/configstore.h>
#include <core/bus.h>
#include <rom/rom.h>

void SRAM::loadSRAM(void)
{
    using namespace boost::filesystem;

    path srampath(ConfigStore::getInstance().getString(GlobalStrings::CFG_SECTION_CORE, "SavePath") + Bus::rom->getRomFilenameNoExtension() + ".sra");

    if (!exists(srampath.parent_path()))
    {
        create_directories(srampath.parent_path());
    }

    if (!exists(srampath))
    {
        _sramfile.open(srampath, std::ios::out);
        _sramfile.seekp(0x7fff);
        _sramfile.put('\0');
        _sramfile.close();
    }

    _sramfile.open(srampath, std::ios::in | std::ios::out | std::ios::binary);
    _sramfile.seekg(0, std::ios::beg);
    _sramfile.seekp(0, std::ios::beg);
}

void SRAM::dmaToSRAM(uint8_t* src, int32_t offset, int32_t len)
{
    if (!_sramfile.is_open())
    {
        loadSRAM();
    }

    _sramfile.seekp(offset, std::ios::beg);
    _sramfile.write((char*)src, len);
    _sramfile.flush();
}

void SRAM::dmaFromSRAM(uint8_t* dest, int32_t offset, int32_t len)
{
    if (!_sramfile.is_open())
    {
        loadSRAM();
    }

    _sramfile.seekg(offset, std::ios::beg);
    _sramfile.read((char*)dest, len);
}

SRAM::~SRAM(void)
{
    if (_sramfile.is_open())
    {
        _sramfile.close();
    }
}

