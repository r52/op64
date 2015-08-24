#include <oputil.h>

#include "videointerface.h"

#include <rcp/rcp.h>
#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <plugin/plugincontainer.h>
#include <plugin/gfxplugin.h>
#include <cpu/interrupthandler.h>
#include <ui/corecontrol.h>

OPStatus VideoInterface::read(Bus* bus, uint32_t address, uint32_t* data)
{
    uint32_t regnum = VI_REG(address);

    if (regnum == VI_CURRENT_REG)
    {
        bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());
        reg[VI_CURRENT_REG] = (Bus::state.vi_delay - (Bus::state.next_vi - Bus::state.cp0_reg[CP0_COUNT_REG])) / CoreControl::VIRefreshRate;
        reg[VI_CURRENT_REG] = (reg[VI_CURRENT_REG] & (~1)) | Bus::state.vi_field;

    }

    *data = reg[regnum];

    return OP_OK;
}

OPStatus VideoInterface::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = VI_REG(address);

    switch (regnum)
    {
    case VI_STATUS_REG:
        if ((reg[VI_STATUS_REG] & mask) != (data & mask))
        {
            masked_write(&reg[VI_STATUS_REG], data, mask);
            if (bus->plugins->gfx()->ViStatusChanged != nullptr)
            {
                bus->plugins->gfx()->ViStatusChanged();
            }
        }
        return OP_OK;

    case VI_WIDTH_REG:
        if ((reg[VI_WIDTH_REG] & mask) != (data & mask))
        {
            masked_write(&reg[VI_WIDTH_REG], data, mask);
            if (bus->plugins->gfx()->ViWidthChanged != nullptr)
            {
                bus->plugins->gfx()->ViWidthChanged();
            }
        }
        return OP_OK;

    case VI_CURRENT_REG:
        Bus::rcp.mi.reg[MI_INTR_REG] &= ~0x8;
        bus->interrupt->checkInterrupt();
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    return OP_OK;
}
