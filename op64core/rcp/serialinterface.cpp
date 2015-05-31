#include <oputil.h>
#include <oplog.h>

#include "serialinterface.h"

#include <rcp/rcp.h>
#include <core/bus.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <pif/pif.h>
#include <ui/configstore.h>


void SerialInterface::DMARead(void)
{
    using namespace Bus;
    if (rcp->si.reg[SI_PIF_ADDR_RD64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR(SerialInterface) << "Unknown SI use";
        Bus::stop = true;
    }

    Bus::pif->pifRead();

    for (uint32_t i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        rdram->mem[(rcp->si.reg[SI_DRAM_ADDR_REG] + i) / 4] = byteswap_u32(*(uint32_t*)&pif->ram[i]);
    }

    Bus::cpu->getCp0()->updateCount(*Bus::PC);

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        Bus::interrupt->addInterruptEvent(SI_INT, /*0x100*/0x900);
    }
    else {
        rcp->mi.reg[MI_INTR_REG] |= 0x02; // SI
        rcp->si.reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        Bus::interrupt->checkInterrupt();
    }
}

void SerialInterface::DMAWrite(void)
{
    using namespace Bus;
    if (rcp->si.reg[SI_PIF_ADDR_WR64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR(SerialInterface) << "Unknown SI use";
        Bus::stop = true;
    }

    for (uint32_t i = 0; i < PIF_RAM_SIZE; i += 4)
    {
        *((uint32_t*)(&pif->ram[i])) = byteswap_u32(rdram->mem[(rcp->si.reg[SI_DRAM_ADDR_REG] + i) / 4]);
    }

    Bus::pif->pifWrite();
    Bus::cpu->getCp0()->updateCount(*Bus::PC);

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        Bus::interrupt->addInterruptEvent(SI_INT, /*0x100*/0x900);
    }
    else {
        rcp->mi.reg[MI_INTR_REG] |= 0x02; // SI
        rcp->si.reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        Bus::interrupt->checkInterrupt();
    }
}

OPStatus SerialInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[SI_REG(address)];
    return OP_OK;
}

OPStatus SerialInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = SI_REG(address);

    switch (regnum)
    {
    case SI_DRAM_ADDR_REG:
        masked_write(&reg[SI_DRAM_ADDR_REG], data, mask);
        break;

    case SI_PIF_ADDR_RD64B_REG:
        masked_write(&reg[SI_PIF_ADDR_RD64B_REG], data, mask);
        DMARead();
        break;

    case SI_PIF_ADDR_WR64B_REG:
        masked_write(&reg[SI_PIF_ADDR_WR64B_REG], data, mask);
        DMAWrite();
        break;

    case SI_STATUS_REG:
        reg[SI_STATUS_REG] &= ~0x1000;
        Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x2;
        Bus::interrupt->checkInterrupt();
        break;
    }

    return OP_OK;
}
