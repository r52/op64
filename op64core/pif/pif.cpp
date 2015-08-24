#include <ctime>

#include <oplog.h>
#include <oputil.h>

#include "pif.h"
#include "n64_cic_nus_6105.h"

#include <core/bus.h>
#include <rom/rom.h>
#include <plugin/plugincontainer.h>
#include <plugin/inputplugin.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <rcp/rcpcommon.h>


PIF::~PIF(void)
{
    uninitialize();
}


bool PIF::initialize(void)
{
    for (uint32_t i = 0; i < 4; i++)
    {
        Bus::state.controllers[i].Present = 0;
        Bus::state.controllers[i].RawData = 0;
        Bus::state.controllers[i].Plugin = PLUGIN_NONE;
    }

    _eeprom.reset(new EEPROM);
    _mempak.reset(new MemPak);

    return true;
}

void PIF::uninitialize(void)
{
    _eeprom.reset();
    _mempak.reset();
}

void PIF::pifRead(Bus* bus)
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
                    if (Bus::state.controllers[channel].Present &&
                        Bus::state.controllers[channel].RawData)
                    {
                        if (bus->plugins->input()->ReadController != nullptr)
                        {
                            bus->plugins->input()->ReadController(channel, &ram[i]);
                        }
                    }
                    else
                    {
                        readController(bus, channel, &ram[i]);
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
    if (bus->plugins->input()->ReadController != nullptr)
    {
        bus->plugins->input()->ReadController(-1, NULL);
    }
}

void PIF::pifWrite(Bus* bus)
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
            LOG_WARNING(PIF) << "Error in write: " << std::hex << ram[0x3F];
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
                    if (Bus::state.controllers[channel].Present &&
                        Bus::state.controllers[channel].RawData)
                    {
                        if (bus->plugins->input()->ControllerCommand != nullptr)
                        {
                            bus->plugins->input()->ControllerCommand(channel, &ram[i]);
                        }
                    }
                    else
                    {
                        controllerCommand(bus, channel, &ram[i]);
                    }
                }
                else if (channel == 4)
                {
                    _eeprom->eepromCommand(bus->rom.get(), &ram[i]);
                }
                else
                {
                    LOG_WARNING(PIF) << "Channel >= 4";
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

    if (bus->plugins->input()->ControllerCommand != nullptr) {
        bus->plugins->input()->ControllerCommand(-1, NULL);
    }
}

void PIF::readController(Bus* bus, int32_t controller, uint8_t* cmd)
{
    switch (cmd[2])
    {
    case 1:
        if (Bus::state.controllers[controller].Present)
        {
            // TODO: pj64 polls getkeys at every VI and
            // simply copies the most recent values here.
            // which is better??
            BUTTONS Keys;
            bus->plugins->input()->GetKeys(controller, &Keys);
            *((unsigned int *)(cmd + 3)) = Keys.Value;
        }
        break;
    case 2: // read controller pack
        if (Bus::state.controllers[controller].Present)
        {
            if (Bus::state.controllers[controller].Plugin == PLUGIN_RAW)
            {
                if (bus->plugins->input()->ReadController != nullptr) {
                    bus->plugins->input()->ReadController(controller, cmd);
                }
            }
        }
        break;
    case 3: // write controller pack
        if (Bus::state.controllers[controller].Present)
        {
            if (Bus::state.controllers[controller].Plugin == PLUGIN_RAW)
            {
                if (bus->plugins->input()->ReadController != nullptr) {
                    bus->plugins->input()->ReadController(controller, cmd);
                }
            }
        }
        break;
    }
}

void PIF::controllerCommand(Bus* bus, int32_t controller, uint8_t* cmd)
{
    switch (cmd[2])
    {
    case 0x00: // read status
    case 0xFF: // reset
        if ((cmd[1] & 0x80))
        {
            break;
        }

        if (Bus::state.controllers[controller].Present)
        {
            cmd[3] = 0x05;
            cmd[4] = 0x00;
            switch (Bus::state.controllers[controller].Plugin)
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
        if (!Bus::state.controllers[controller].Present)
        {
            cmd[1] |= 0x80;
        }
        break;
    case 0x02: // read controller pack
        if (Bus::state.controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (Bus::state.controllers[controller].Plugin)
            {
            case PLUGIN_MEMPAK:
                _mempak->read(bus->rom.get(), controller, address, &cmd[5]);
                break;
            case PLUGIN_RAW:
                if (bus->plugins->input()->ControllerCommand != nullptr)
                {
                    bus->plugins->input()->ControllerCommand(controller, cmd);
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
        if (Bus::state.controllers[controller].Present)
        {
            int address = (cmd[3] << 8) | cmd[4];
            switch (Bus::state.controllers[controller].Plugin)
            {
            case PLUGIN_MEMPAK:
                _mempak->write(bus->rom.get(), controller, address, &cmd[5]);
                break;
            case PLUGIN_RAW:
                if (bus->plugins->input()->ControllerCommand != nullptr)
                {
                    bus->plugins->input()->ControllerCommand(controller, cmd);
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

OPStatus PIF::read(Bus* bus, uint32_t address, uint32_t* data)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR(PIF) << "Reading a byte in PIF at invalid address 0x" << std::hex << address;
        *data = 0;
        return OP_ERROR;
    }

    *data = byteswap_u32(*(uint32_t*)(ram + addr));

    return OP_OK;
}

OPStatus PIF::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t addr = PIF_ADDRESS(address);

    if (addr >= PIF_RAM_SIZE)
    {
        LOG_ERROR(PIF) << "Invalid PIF address: " << std::hex << address;
        return OP_ERROR;
    }

    masked_write((uint32_t*)(&ram[addr]), byteswap_u32(data), byteswap_u32(mask));

    if ((addr == 0x3c) && (mask & 0xff))
    {
        if (ram[0x3f] == 0x08)
        {
            ram[0x3f] = 0;
            bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());
            bus->interrupt->addInterruptEvent(SI_INT, 0x900);
        }
        else
        {
            pifWrite(bus);
        }
    }

    return OP_OK;
}

