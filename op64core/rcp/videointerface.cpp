#include <oputil.h>

#include "videointerface.h"

#include <rcp/rcp.h>
#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <plugin/plugincontainer.h>
#include <plugin/gfxplugin.h>
#include <cpu/interrupthandler.h>

OPStatus VideoInterface::read(uint32_t address, uint32_t* data)
{
    uint32_t regnum = VI_REG(address);

    if (regnum == VI_CURRENT_REG)
    {
        Bus::cpu->getCp0()->updateCount(*Bus::PC);
        if (reg[VI_V_SYNC_REG])
        {
            reg[VI_CURRENT_REG] = (Bus::vi_delay - (Bus::next_vi - Bus::cp0_reg[CP0_COUNT_REG])) / (Bus::vi_delay / reg[VI_V_SYNC_REG]);
            reg[VI_CURRENT_REG] = (reg[VI_CURRENT_REG] & (~1)) | Bus::vi_field;
        }
        else
        {
            reg[VI_CURRENT_REG] = 0;
        }
    }

    *data = reg[regnum];

    return OP_OK;
}

OPStatus VideoInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = VI_REG(address);

    switch (regnum)
    {
    case VI_STATUS_REG:
        if ((reg[VI_STATUS_REG] & mask) != (data & mask))
        {
            masked_write(&reg[VI_STATUS_REG], data, mask);
            if (Bus::plugins->gfx()->ViStatusChanged != nullptr)
            {
                Bus::plugins->gfx()->ViStatusChanged();
            }
        }
        return OP_OK;

    case VI_WIDTH_REG:
        if ((reg[VI_WIDTH_REG] & mask) != (data & mask))
        {
            masked_write(&reg[VI_WIDTH_REG], data, mask);
            if (Bus::plugins->gfx()->ViWidthChanged != nullptr)
            {
                Bus::plugins->gfx()->ViWidthChanged();
            }
        }
        return OP_OK;

    case VI_CURRENT_REG:
        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x8;
        Bus::interrupt->checkInterrupt();
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);

    return OP_OK;
}
