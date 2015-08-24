#include <oputil.h>

#include "audiointerface.h"

#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <plugin/plugincontainer.h>
#include <plugin/audioplugin.h>
#include <rom/rom.h>
#include <rcp/rcp.h>


OPStatus AudioInterface::read(Bus* bus, uint32_t address, uint32_t* data)
{
    uint32_t regnum = AI_REG(address);

    if (regnum == AI_LEN_REG)
    {
        bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());
        if (fifo[0].delay != 0 && bus->interrupt->findEvent(AI_INT) != 0 && (bus->interrupt->findEvent(AI_INT) - Bus::state.cp0_reg[CP0_COUNT_REG]) < 0x80000000)
        {
            *data = ((bus->interrupt->findEvent(AI_INT) - Bus::state.cp0_reg[CP0_COUNT_REG])*(int64_t)fifo[0].length) / fifo[0].delay;
        }
        else
        {
            *data = 0;
        }
    }
    else
    {
        *data = reg[regnum];
    }

    return OP_OK;
}

OPStatus AudioInterface::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = AI_REG(address);

    unsigned int freq, delay = 0;
    switch (regnum)
    {
    case AI_LEN_REG:
        masked_write(&reg[AI_LEN_REG], data, mask);
        if (bus->plugins->audio()->LenChanged != nullptr)
        {
            bus->plugins->audio()->LenChanged();
        }

        freq = bus->rom->getAiDACRate() / (reg[AI_DACRATE_REG] + 1);
        if (freq)
            delay = (unsigned int)(((uint64_t)reg[AI_LEN_REG] * Bus::state.vi_delay * bus->rom->getViLimit()) / (freq * 4));

        if (reg[AI_STATUS_REG] & 0x40000000) // busy
        {
            fifo[1].delay = delay;
            fifo[1].length = reg[AI_LEN_REG];
            reg[AI_STATUS_REG] |= 0x80000000;
        }
        else
        {
            fifo[0].delay = delay;
            fifo[0].length = reg[AI_LEN_REG];
            bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());
            bus->interrupt->addInterruptEvent(AI_INT, delay);
            reg[AI_STATUS_REG] |= 0x40000000;
        }
        return OP_OK;

    case AI_STATUS_REG:
        Bus::rcp.mi.reg[MI_INTR_REG] &= ~0x4;
        bus->interrupt->checkInterrupt();
        return OP_OK;

    case AI_DACRATE_REG:
        if ((reg[AI_DACRATE_REG] & mask) != (data & mask))
        {
            masked_write(&reg[AI_DACRATE_REG], data, mask);
            bus->plugins->audio()->DacrateChanged(bus->rom->getSystemType());
        }
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    return OP_OK;
}
