#include <ctime>

#include "pif.h"
#include "bus.h"
#include "logger.h"
#include "n64_cic_nus_6105.h"
#include "rom.h"

#include "plugins.h"
#include "inputplugin.h"

#include "util.h"
#include "icpu.h"
#include "cp0.h"
#include "interrupthandler.h"


PIF::PIF(void) :
_eeprom(nullptr),
_mempak(nullptr)
{
}

PIF::~PIF(void)
{
    uninitialize();
}


void PIF::initialize(void)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        Bus::controllers[i].Present = 0;
        Bus::controllers[i].RawData = 0;
        Bus::controllers[i].Plugin = PLUGIN_NONE;
    }

    uninitialize();

    _eeprom = new EEPROM;
    _mempak = new MemPak;
}

void PIF::uninitialize(void)
{
    if (_eeprom != nullptr)
    {
        delete _eeprom; _eeprom = nullptr;
    }

    if (_mempak != nullptr)
    {
        delete _mempak; _mempak = nullptr;
    }
}

void PIF::pifRead(void)
{
    int32_t i = 0, channel = 0;
    while (i < 0x40)
    {
        switch (ram[i])
        {
        case 0x00:
            channel++;
            if (channel > 6)
            {
                i = 0x40;
            }
            break;
        case 0xFE:
            i = 0x40;
            break;
        case 0xFF:
            break;
        case 0xB4:
        case 0x56:
        case 0xB8:
            break;
        default:
            if (!(ram[i] & 0xC0))
            {
                if (channel < 4)
                {
                    if (Bus::controllers[channel].Present &&
                        Bus::controllers[channel].RawData)
                    {
                        if (Bus::plugins->input()->ReadController != nullptr)
                        {
                            Bus::plugins->input()->ReadController(channel, &ram[i]);
                        }
                    }
                    else
                    {
                        readController(channel, &ram[i]);
                    }
                }
                i += ram[i] + (ram[(i + 1)] & 0x3F) + 1;
                channel++;
            }
            else
            {
                i = 0x40;
            }
        }
        i++;
    }
    if (Bus::plugins->input()->ReadController != nullptr)
    {
        Bus::plugins->input()->ReadController(-1, NULL);
    }
}

void PIF::pifWrite(void)
{
    int i = 0, channel = 0;
    if (ram[0x3F] > 1)
    {
        switch (ram[0x3F])
        {
        case 0x02:
            char challenge[30], response[30];
            // format the 'challenge' message into 30 nibbles for X-Scale's CIC code
            for (i = 0; i < 15; i++)
            {
                challenge[i * 2] = (ram[48 + i] >> 4) & 0x0f;
                challenge[i * 2 + 1] = ram[48 + i] & 0x0f;
            }
            // calculate the proper response for the given challenge (X-Scale's algorithm)
            n64_cic_nus_6105(challenge, response, CHL_LEN - 2);
            ram[46] = 0;
            ram[47] = 0;
            // re-format the 'response' into a byte stream
            for (i = 0; i < 15; i++)
            {
                ram[48 + i] = (response[i * 2] << 4) + response[i * 2 + 1];
            }
            // the last byte (2 nibbles) is always 0
            ram[63] = 0;
            break;
        case 0x08:
            ram[0x3F] = 0;
            break;
        default:
            LOG_WARNING("%s: error in write: %x", __FUNCTION__, ram[0x3F]);
            break;
        }
        return;
    }
    while (i < 0x40)
    {
        switch (ram[i])
        {
        case 0x00:
            channel++;
            if (channel > 6)
            {
                i = 0x40;
            }
            break;
        case 0xFF:
            break;
        default:
            if (!(ram[i] & 0xC0))
            {
                if (channel < 4)
                {
                    if (Bus::controllers[channel].Present &&
                        Bus::controllers[channel].RawData)
                    {
                        if (Bus::plugins->input()->ControllerCommand != nullptr)
                        {
                            Bus::plugins->input()->ControllerCommand(channel, &ram[i]);
                        }
                    }
                    else
                    {
                        controllerCommand(channel, &ram[i]);
                    }
                }
                else if (channel == 4)
                {
                    _eeprom->eepromCommand(&ram[i]);
                }
                else
                {
                    LOG_WARNING("%s: channel >= 4", __FUNCTION__);
                }

                i += ram[i] + (ram[(i + 1)] & 0x3F) + 1;
                channel++;
            }
            else
            {
                i = 0x40;
            }
        }
        i++;
    }
    //PIF_RAMb[0x3F] = 0;

    if (Bus::plugins->input()->ControllerCommand != nullptr) {
        Bus::plugins->input()->ControllerCommand(-1, NULL);
    }
}

void PIF::readController(int32_t controller, uint8_t* cmd)
{
    switch (cmd[2])
    {
    case 1:
        if (Bus::controllers[controller].Present)
        {
            // TODO: pj64 polls getkeys at every VI and
            // simply copies the most recent values here.
            // which is better??
            BUTTONS Keys;
            Bus::plugins->input()->GetKeys(controller, &Keys);
            *((unsigned int *)(cmd + 3)) = Keys.Value;
        }
        break;
    case 2: // read controller pack
        if (Bus::controllers[controller].Present)
        {
            if (Bus::controllers[controller].Plugin == PLUGIN_RAW)
            {
                if (Bus::plugins->input()->ReadController != nullptr) {
                    Bus::plugins->input()->ReadController(controller, cmd);
                }
            }
        }
        break;
    case 3: // write controller pack
        if (Bus::controllers[controller].Present)
        {
            if (Bus::controllers[controller].Plugin == PLUGIN_RAW)
            {
                if (Bus::plugins->input()->ReadController != nullptr) {
                    Bus::plugins->input()->ReadController(controller, cmd);
                }
            }
        }
        break;
    }
}

void PIF::controllerCommand(int32_t controller, uint8_t* cmd)
{
    switch (cmd[2])
    {
    case 0x00: // read status
    case 0xFF: // reset
        if ((cmd[1] & 0x80))
        {
            break;
        }

        if (Bus::controllers[controller].Present)
        {
            cmd[3] = 0x05;
            cmd[4] = 0x00;
            switch (Bus::controllers[controller].Plugin)
            {
            case PLUGIN_MEMPAK:
                cmd[5] = 1;
                break;
            case PLUGIN_RAW:
                cmd[5] = 1;
                break;
            default:
                cmd[5] = 0;
                break;
            }
        }
        else
        {
            cmd[1] |= 0x80;
        }
        break;
    case 0x01:
        if (!Bus::controllers[controller].Present)
        {
            cmd[1] |= 0x80;
        }
        break;
    case 0x02: // read controller pack
        if (Bus::controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (Bus::controllers[controller].Plugin)
            {
            case PLUGIN_MEMPAK:
                _mempak->read(controller, address, &cmd[5]);
                break;
            case PLUGIN_RAW:
                if (Bus::plugins->input()->ControllerCommand != nullptr)
                {
                    Bus::plugins->input()->ControllerCommand(controller, cmd);
                }
                break;
            default:
                memset(&cmd[5], 0, 0x20);
                cmd[0x25] = 0;
            }
        }
        else
        {
            cmd[1] |= 0x80;
        }
        break;
    case 0x03: // write controller pack
        if (Bus::controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (Bus::controllers[controller].Plugin)
            {
            case PLUGIN_MEMPAK:
                _mempak->write(controller, address, &cmd[5]);
                break;
            case PLUGIN_RAW:
                if (Bus::plugins->input()->ControllerCommand != nullptr)
                {
                    Bus::plugins->input()->ControllerCommand(controller, cmd);
                }
                break;
            default:
                cmd[0x25] = MemPak::calculateCRC(&cmd[5]);
            }
        }
        else
        {
            cmd[1] |= 0x80;
        }
        break;
    }
}

OPStatus PIF::read(uint32_t address, uint32_t* data)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR("Reading a byte in PIF at invalid address 0x%x", address);
        *data = 0;
        return OP_ERROR;
    }

    *data = byteswap_u32(*(uint32_t*)(ram + addr));

    return OP_OK;
}

OPStatus PIF::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR("Invalid PIF address: %08x", address);
        return OP_ERROR;
    }

    masked_write((uint32_t*)(&ram[addr]), byteswap_u32(data), byteswap_u32(mask));

    if ((addr == 0x3c) && (mask & 0xff))
    {
        if (ram[0x3f] == 0x08)
        {
            ram[0x3f] = 0;
            Bus::cpu->getCp0()->updateCount(*Bus::PC);
            Bus::interrupt->addInterruptEvent(SI_INT, 0x900);
        }
        else
        {
            pifWrite();
        }
    }

    return OP_OK;
}

