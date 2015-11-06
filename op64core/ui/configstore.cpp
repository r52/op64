#include "configstore.h"

#include <globalstrings.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <boost/property_tree/ini_parser.hpp>


void ConfigStore::loadConfig(void)
{
    boost::filesystem::path _configpath(GlobalStrings::CFG_FILENAME);

    bool fileExists = boost::filesystem::exists(_configpath);

    if (!fileExists)
    {
        boost::filesystem::fstream _configstream(_configpath, std::ios::out);
        _configstream.close();
    }

    boost::property_tree::ini_parser::read_ini(GlobalStrings::CFG_FILENAME, pt);

    if (!fileExists)
    {
        // set up some default values
        setupDefaults();
    }
}

void ConfigStore::saveConfig(void)
{
    boost::property_tree::ini_parser::write_ini(GlobalStrings::CFG_FILENAME, pt);
}

ConfigStore::ConfigStore(void)
{
    loadConfig();
}

ConfigStore::~ConfigStore(void)
{
    saveConfig();
}

void ConfigStore::setupDefaults(void)
{
    using namespace GlobalStrings;
    set(CFG_SECTION_CORE, CFG_DELAY_SI, false);
    set(CFG_SECTION_CORE, CFG_SAVE_PATH, CFG_SAVE_PATH_DEFAULT);
    set(CFG_SECTION_CORE, CFG_GFX_PATH, CFG_GFX_PATH_DEFAULT);
    set(CFG_SECTION_CORE, CFG_AUDIO_PATH, CFG_AUDIO_PATH_DEFAULT);
    set(CFG_SECTION_CORE, CFG_RSP_PATH, CFG_RSP_PATH_DEFAULT);
    set(CFG_SECTION_CORE, CFG_INPUT_PATH, CFG_INPUT_PATH_DEFAULT);
}
