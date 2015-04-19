#include <util.h>

#include "audiointerface.h"

#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <plugin/plugins.h>
#include <plugin/audioplugin.h>
#include <rom/rom.h>
#include <rcp/rcp.h>


OPStatus AudioInterface::read(uint32_t address, uint32_t* data)
{
    uint32_t regnum = AI_REG(address);

    if (regnum == AI_LEN_REG)
    {
        Bus::cpu->getCp0()->updateCount(*Bus::PC);
        if (fifo[0].delay != 0 && Bus::interrupt->findEvent(AI_INT) != 0 && (Bus::interrupt->findEvent(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG]) < 0x80000000)
        {
            *data = ((Bus::interrupt->findEvent(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG])*(int64_t)fifo[0].length) / fifo[0].delay;
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

OPStatus AudioInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = AI_REG(address);

    unsigned int freq, delay = 0;
    switch (regnum)
    {
    case AI_LEN_REG:
        masked_write(&reg[AI_LEN_REG], data, mask);
        if (Bus::plugins->audio()->LenChanged != nullptr)
        {
            Bus::plugins->audio()->LenChanged();
        }

        freq = Bus::rom->getAiDACRate() / (reg[AI_DACRATE_REG] + 1);
        if (freq)
            delay = (unsigned int)(((uint64_t)reg[AI_LEN_REG] * Bus::vi_delay * Bus::rom->getViLimit()) / (freq * 4));

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
            Bus::cpu->getCp0()->updateCount(*Bus::PC);
            Bus::interrupt->addInterruptEvent(AI_INT, delay);
            reg[AI_STATUS_REG] |= 0x40000000;
        }
        return OP_OK;

    case AI_STATUS_REG:
        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x4;
        Bus::interrupt->checkInterrupt();
        return OP_OK;

    case AI_DACRATE_REG:
        if ((reg[AI_DACRATE_REG] & mask) != (data & mask))
        {
            masked_write(&reg[AI_DACRATE_REG], data, mask);
            Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
        }
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    return OP_OK;
}
