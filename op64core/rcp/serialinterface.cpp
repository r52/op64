#include <oputil.h>
#include <oplog.h>

#include "serialinterface.h"

#include <globalstrings.h>
#include <rcp/rcp.h>
#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <pif/pif.h>
#include <ui/configstore.h>
#include <ui/corecontrol.h>

using namespace GlobalStrings;

void SerialInterface::DMARead(Bus* bus)
{
    if (Bus::rcp.si.reg[SI_PIF_ADDR_RD64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR(SerialInterface) << "Unknown SI use";
        CoreControl::stop = true;
    }

    bus->pif->pifRead(bus);

    for (uint32_t i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        Bus::rdram.mem[(Bus::rcp.si.reg[SI_DRAM_ADDR_REG] + i) / 4] = byteswap_u32(*(uint32_t*)&bus->pif->ram[i]);
    }

    bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        bus->interrupt->addInterruptEvent(SI_INT, /*0x100*/0x900);
    }
    else {
        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x02; // SI
        Bus::rcp.si.reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        bus->interrupt->checkInterrupt();
    }
}

void SerialInterface::DMAWrite(Bus* bus)
{
    if (Bus::rcp.si.reg[SI_PIF_ADDR_WR64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR(SerialInterface) << "Unknown SI use";
        CoreControl::stop = true;
    }

    for (uint32_t i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        *((uint32_t*)(&bus->pif->ram[i])) = byteswap_u32(Bus::rdram.mem[(Bus::rcp.si.reg[SI_DRAM_ADDR_REG] + i) / 4]);
    }

    bus->pif->pifWrite(bus);
    bus->cpu->getCP0().updateCount(Bus::state.PC, bus->rom->getCountPerOp());

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        bus->interrupt->addInterruptEvent(SI_INT, /*0x100*/0x900);
    }
    else {
        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x02; // SI
        Bus::rcp.si.reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        bus->interrupt->checkInterrupt();
    }
}

OPStatus SerialInterface::read(Bus* bus, uint32_t address, uint32_t* data)
{
    *data = reg[SI_REG(address)];
    return OP_OK;
}

OPStatus SerialInterface::write(Bus* bus, uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = SI_REG(address);

    switch (regnum)
    {
    case SI_DRAM_ADDR_REG:
        masked_write(&reg[SI_DRAM_ADDR_REG], data, mask);
        break;

    case SI_PIF_ADDR_RD64B_REG:
        masked_write(&reg[SI_PIF_ADDR_RD64B_REG], data, mask);
        DMARead(bus);
        break;

    case SI_PIF_ADDR_WR64B_REG:
        masked_write(&reg[SI_PIF_ADDR_WR64B_REG], data, mask);
        DMAWrite(bus);
        break;

    case SI_STATUS_REG:
        reg[SI_STATUS_REG] &= ~0x1000;
        Bus::rcp.mi.reg[MI_INTR_REG] &= ~0x2;
        bus->interrupt->checkInterrupt();
        break;
    }

    return OP_OK;
}
