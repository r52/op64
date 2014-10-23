#include "pif.h"
#include "bus.h"
#include "logger.h"
#include "n64_cic_nus_6105.h"
#include "rom.h"
#include <ctime>
#include "plugins.h"
#include "inputplugin.h"


PIF::PIF(void) :
_eeprom(new EEPROM),
_mempak(new MemPak)
{
    Bus::controllers = _controllers;
}

PIF::~PIF(void)
{
    if (_eeprom != nullptr)
    {
        delete _eeprom; _eeprom = nullptr;
    }

    if (_mempak != nullptr)
    {
        delete _mempak; _mempak = nullptr;
    }

    Bus::controllers = nullptr;
}


void PIF::initialize(void)
{
    vec_for (uint32_t i = 0; i < 4; i++)
    {
        _controllers[i].Present = 0;
        _controllers[i].RawData = 0;
        _controllers[i].Plugin = PLUGIN_NONE;
    }
}

void PIF::pifRead(void)
{
    int32_t i = 0, channel = 0;
    while (i < 0x40)
    {
        switch (Bus::pif_ram8[i])
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
            if (!(Bus::pif_ram8[i] & 0xC0))
            {
                if (channel < 4)
                {
                    if (_controllers[channel].Present &&
                        _controllers[channel].RawData)
                    {
                        if (Bus::plugins->input()->ReadController != nullptr)
                        {
                            Bus::plugins->input()->ReadController(channel, &Bus::pif_ram8[i]);
                        }
                    }
                    else
                    {
                        readController(channel, &Bus::pif_ram8[i]);
                    }
                }
                i += Bus::pif_ram8[i] + (Bus::pif_ram8[(i + 1)] & 0x3F) + 1;
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
    char challenge[30], response[30];
    int i = 0, channel = 0;
    if (Bus::pif_ram8[0x3F] > 1)
    {
        switch (Bus::pif_ram8[0x3F])
        {
        case 0x02:
            // format the 'challenge' message into 30 nibbles for X-Scale's CIC code
            for (i = 0; i < 15; i++)
            {
                challenge[i * 2] = (Bus::pif_ram8[48 + i] >> 4) & 0x0f;
                challenge[i * 2 + 1] = Bus::pif_ram8[48 + i] & 0x0f;
            }
            // calculate the proper response for the given challenge (X-Scale's algorithm)
            n64_cic_nus_6105(challenge, response, CHL_LEN - 2);
            Bus::pif_ram8[46] = 0;
            Bus::pif_ram8[47] = 0;
            // re-format the 'response' into a byte stream
            for (i = 0; i < 15; i++)
            {
                Bus::pif_ram8[48 + i] = (response[i * 2] << 4) + response[i * 2 + 1];
            }
            // the last byte (2 nibbles) is always 0
            Bus::pif_ram8[63] = 0;
            break;
        case 0x08:
            Bus::pif_ram8[0x3F] = 0;
            break;
        default:
            LOG_ERROR("%s: error in write: %x", __FUNCTION__, Bus::pif_ram8[0x3F]);
            break;
        }
        return;
    }
    while (i < 0x40)
    {
        switch (Bus::pif_ram8[i])
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
            if (!(Bus::pif_ram8[i] & 0xC0))
            {
                if (channel < 4)
                {
                    if (_controllers[channel].Present &&
                        _controllers[channel].RawData)
                    {
                        if (Bus::plugins->input()->ControllerCommand != nullptr)
                        {
                            Bus::plugins->input()->ControllerCommand(channel, &Bus::pif_ram8[i]);
                        }
                    }
                    else
                    {
                        controllerCommand(channel, &Bus::pif_ram8[i]);
                    }
                }
                else if (channel == 4)
                {
                    _eeprom->eepromCommand(&Bus::pif_ram8[i]);
                }
                else
                {
                    LOG_ERROR("%s: channel >= 4", __FUNCTION__);
                }

                i += Bus::pif_ram8[i] + (Bus::pif_ram8[(i + 1)] & 0x3F) + 1;
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
        if (_controllers[controller].Present)
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
        if (_controllers[controller].Present)
        {
            if (_controllers[controller].Plugin == PLUGIN_RAW)
            {
                if (Bus::plugins->input()->ReadController != nullptr) {
                    Bus::plugins->input()->ReadController(controller, cmd);
                }
            }
        }
        break;
    case 3: // write controller pack
        if (_controllers[controller].Present)
        {
            if (_controllers[controller].Plugin == PLUGIN_RAW)
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

        if (_controllers[controller].Present)
        {
            cmd[3] = 0x05;
            cmd[4] = 0x00;
            switch (_controllers[controller].Plugin)
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
        if (!_controllers[controller].Present)
        {
            cmd[1] |= 0x80;
        }
        break;
    case 0x02: // read controller pack
        if (_controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (_controllers[controller].Plugin)
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
        if (_controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (_controllers[controller].Plugin)
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

