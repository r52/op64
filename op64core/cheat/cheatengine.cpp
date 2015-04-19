#include <strutil.h>
#include <compiler.h>
#include <logger.h>
#include <util.h>

#include "cheatengine.h"

#include <core/bus.h>
#include <rcp/rdramcontroller.h>


#define CHEAT_CODE_MAGIC_VALUE 0xDEADBEEF


static uint16_t read_address_16bit(uint32_t address)
{
    return *(uint16_t*)(((uint8_t*)Bus::rdram->mem + (HES(address & 0xFFFFFF))));
}

static uint8_t read_address_8bit(uint32_t address)
{
    return *(uint8_t*)(((uint8_t*)Bus::rdram->mem + (BES(address & 0xFFFFFF))));
}

static void update_address_16bit(uint32_t address, uint16_t new_value)
{
    *(uint16_t*)(((uint8_t*)Bus::rdram->mem + (HES(address & 0xFFFFFF)))) = new_value;
}

static void update_address_8bit(uint32_t address, uint8_t new_value)
{
    *(uint8_t*)(((uint8_t*)Bus::rdram->mem + (BES(address & 0xFFFFFF)))) = new_value;
}

static bool address_equal_to_8bit(uint32_t address, uint8_t value)
{
    uint8_t value_read;
    value_read = *(uint8_t *)(((uint8_t*)Bus::rdram->mem + (BES(address & 0xFFFFFF))));
    return value_read == value;
}

static bool address_equal_to_16bit(uint32_t address, uint16_t value)
{
    uint16_t value_read;
    value_read = *(uint16_t*)(((uint8_t*)Bus::rdram->mem + (HES(address & 0xFFFFFF))));
    return value_read == value;
}

CheatEngine::CheatEngine()
{

}

CheatEngine::~CheatEngine()
{

}

CheatCodeList CheatEngine::toCheatCodeList(const std::string& strcodelist)
{
    std::vector<std::string> strcodes = split(strcodelist, ',');

    CheatCodeList templist;

    for (std::string code : strcodes)
    {
        CheatCode newcode;
        if (_s_sscanf(code.c_str(), "%08X %04X", &newcode.address, &newcode.value) == 2)
        {
            newcode.old_value = CHEAT_CODE_MAGIC_VALUE;
            templist.push_back(newcode);
        }
        else
        {
            LOG_ERROR("Cheat: invalid cheat code %s", code.c_str());
        }
    }

    return processCodeList(templist);
}

CheatCodeList CheatEngine::processCodeList(CheatCodeList& rawlist)
{
    CheatCodeList processedlist;

    for (uint32_t i = 0; i < rawlist.size(); i++)
    {
        // if this is a 'patch' code, convert it and dump out all of the individual codes
        if ((rawlist[i].address & 0xFFFF0000) == 0x50000000 && i < rawlist.size() - 1)
        {
            uint32_t code_count = ((rawlist[i].address & 0xFF00) >> 8);
            uint32_t incr_addr = rawlist[i].address & 0xFF;
            int32_t incr_value = rawlist[i].value;
            uint32_t cur_addr = rawlist[i + 1].address;
            int32_t cur_value = rawlist[i + 1].value;
            i += 1;

            for (uint32_t j = 0; j < code_count; j++)
            {
                CheatCode code;
                code.address = cur_addr;
                code.value = cur_value;
                code.old_value = CHEAT_CODE_MAGIC_VALUE;
                processedlist.push_back(code);

                cur_addr += incr_addr;
                cur_value += incr_value;
            }
        }
        else
        {
            // just a normal code
            processedlist.push_back(rawlist[i]);
        }
    }

    return processedlist;
}

void CheatEngine::addRomHacks(CheatList romhacks)
{
    _romhacks = romhacks;
}

void CheatEngine::applyCheats(CheatEntry entry)
{
    // TODO: Fix up for all active cheats. Rom hacks only for now
    for (Cheat cheat : _romhacks)
    {
        switch (entry)
        {
        case ENTRY_BOOT:
            for (CheatCode code : cheat.codes)
            {
                if ((code.address & 0xF0000000) == 0xF0000000)
                {
                    execute_cheat(code.address, code.value, &code.old_value);
                }
            }
            break;
        case ENTRY_VI:
        {
            bool failed = false;
            for (CheatCode code : cheat.codes)
            {
                if ((code.address & 0xF0000000) == 0xD0000000)
                {
                    // if code needs GS button pressed and it's not, skip it
                    if (((code.address & 0xFF000000) == 0xD8000000 ||
                        (code.address & 0xFF000000) == 0xD9000000 ||
                        (code.address & 0xFF000000) == 0xDA000000 ||
                        (code.address & 0xFF000000) == 0xDB000000) &&
                        !_gsButtonPressed)
                        // if condition false, skip next code non-test code
                        failed = true;

                    // if condition false, skip next code non-test code
                    if (!execute_cheat(code.address, code.value, nullptr))
                        failed = true;
                }
                else {
                    /* preconditions were false for this non-test code
                    * reset the condition state and skip the cheat
                    */
                    if (failed) {
                        failed = false;
                        continue;
                    }

                    switch (code.address & 0xFF000000) {
                        // GS button triggers cheat code
                    case 0x88000000:
                    case 0x89000000:
                    case 0xA8000000:
                    case 0xA9000000:
                        if (_gsButtonPressed)
                            execute_cheat(code.address, code.value, nullptr);
                        break;
                        // normal cheat code
                    default:
                        // exclude boot-time cheat codes
                        if ((code.address & 0xF0000000) != 0xF0000000)
                        {
                            execute_cheat(code.address, code.value, &code.old_value);
                        }
                        break;
                    }
                }
            }
        }
            break;
        default:
            break;
        }
    }
}

bool CheatEngine::execute_cheat(uint32_t address, uint16_t value, int32_t* old_value)
{
    switch (address & 0xFF000000)
    {
    case 0x80000000:
    case 0x88000000:
    case 0xA0000000:
    case 0xA8000000:
    case 0xF0000000:
        // if pointer to old value is valid and uninitialized, write current value to it
        if (old_value && (*old_value == CHEAT_CODE_MAGIC_VALUE))
        {
            *old_value = (int32_t)read_address_8bit(address);
        }
        update_address_8bit(address, (uint8_t)value);
        return true;
    case 0x81000000:
    case 0x89000000:
    case 0xA1000000:
    case 0xA9000000:
    case 0xF1000000:
        // if pointer to old value is valid and uninitialized, write current value to it
        if (old_value && (*old_value == CHEAT_CODE_MAGIC_VALUE))
        {
            *old_value = (int32_t)read_address_16bit(address);
        }
        update_address_16bit(address, value);
        return true;
    case 0xD0000000:
    case 0xD8000000:
        return address_equal_to_8bit(address, (uint8_t)value);
    case 0xD1000000:
    case 0xD9000000:
        return address_equal_to_16bit(address, value);
    case 0xD2000000:
    case 0xDB000000:
        return !(address_equal_to_8bit(address, (uint8_t)value));
    case 0xD3000000:
    case 0xDA000000:
        return !(address_equal_to_16bit(address, value));
    case 0xEE000000:
        // most likely, this doesnt do anything.
        execute_cheat(0xF1000318, 0x0040, nullptr);
        execute_cheat(0xF100031A, 0x0000, nullptr);
        return true;
    default:
        return true;
    }

    return true;
}
