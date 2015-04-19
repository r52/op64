#include <util.h>
#include <logger.h>

#include "dma.h"

#include <core/bus.h>
#include <mem/imemory.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <rom/rom.h>
#include <rom/sram.h>
#include <rom/flashram.h>
#include <ui/configstore.h>
#include <pif/pif.h>
#include <rcp/rcpcommon.h>
#include <rcp/rdramcontroller.h>

using namespace Bus;

void DMA::readSP(void)
{
    uint32_t len_reg = rcp->sp.reg[SP_WR_LEN_REG];

    uint32_t length = ((len_reg & 0xfff) | 7) + 1;
    uint32_t count = ((len_reg >> 12) & 0xff) + 1;
    uint32_t skip = ((len_reg >> 20) & 0xfff);

    uint32_t memaddr = rcp->sp.reg[SP_MEM_ADDR_REG] & 0xfff;
    uint32_t dramaddr = rcp->sp.reg[SP_DRAM_ADDR_REG] & 0xffffff;

    uint8_t* spmem = ((rcp->sp.reg[SP_MEM_ADDR_REG] & 0x1000) != 0) ? (uint8_t*)rcp->sp.imem : (uint8_t*)rcp->sp.dmem;
    uint8_t* dram = (uint8_t*)rdram->mem;

    for (uint32_t j = 0; j < count; j++) {
        for (uint32_t i = 0; i < length; i++) {
            dram[BES(dramaddr)] = spmem[BES(memaddr)];
            memaddr++;
            dramaddr++;
        }
        dramaddr += skip;
    }
}

void DMA::writeSP(void)
{
    uint32_t len_reg = rcp->sp.reg[SP_RD_LEN_REG];

    uint32_t length = ((len_reg & 0xfff) | 7) + 1;
    uint32_t count = ((len_reg >> 12) & 0xff) + 1;
    uint32_t skip = ((len_reg >> 20) & 0xfff);

    uint32_t memaddr = rcp->sp.reg[SP_MEM_ADDR_REG] & 0xfff;
    uint32_t dramaddr = rcp->sp.reg[SP_DRAM_ADDR_REG] & 0xffffff;

    uint8_t* spmem = ((rcp->sp.reg[SP_MEM_ADDR_REG] & 0x1000) != 0) ? (uint8_t*)rcp->sp.imem : (uint8_t*)rcp->sp.dmem;
    uint8_t* dram = (uint8_t*)rdram->mem;

    for (uint32_t j = 0; j < count; j++) {
        for (uint32_t i = 0; i < length; i++) {
            spmem[BES(memaddr)] = dram[BES(dramaddr)];
            memaddr++;
            dramaddr++;
        }
        dramaddr += skip;
    }
}

void DMA::readPI(void)
{
    if (rcp->pi.reg[PI_CART_ADDR_REG] >= 0x08000000
        && rcp->pi.reg[PI_CART_ADDR_REG] < 0x08010000)
    {
        if (rom->getSaveType() == SAVETYPE_AUTO)
        {
            rom->setSaveType(SAVETYPE_SRAM);
        }

        if (rom->getSaveType() == SAVETYPE_SRAM)
        {
            sram->dmaToSRAM(
                (uint8_t*)rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000,
                (rcp->pi.reg[PI_RD_LEN_REG] & 0xFFFFFF) + 1
                );
        }

        if (rom->getSaveType() == SAVETYPE_FLASH_RAM)
        {
            flashram->dmaToFlash(
                (uint8_t*)rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000,
                (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                );
        }
    }
    else
    {
        LOG_WARNING("%s: Unknown dma read", __FUNCTION__);
    }

    rcp->pi.reg[PI_STATUS_REG] |= 1;
    cpu->getCp0()->updateCount(*PC);
    interrupt->addInterruptEvent(PI_INT, 0x1000/*rcp->pi.register.pi_rd_len_reg*/);
}

void DMA::writePI(void)
{
    if (rcp->pi.reg[PI_CART_ADDR_REG] < 0x10000000)
    {
        if (rcp->pi.reg[PI_CART_ADDR_REG] >= 0x08000000
            && rcp->pi.reg[PI_CART_ADDR_REG] < 0x08010000)
        {
            if (rom->getSaveType() == SAVETYPE_AUTO)
            {
                rom->setSaveType(SAVETYPE_SRAM);
            }

            if (rom->getSaveType() == SAVETYPE_SRAM)
            {
                sram->dmaFromSRAM(
                    (uint8_t*)rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                    (rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000) & 0xFFFF,
                    (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                    );
            }

            if (rom->getSaveType() == SAVETYPE_FLASH_RAM)
            {
                flashram->dmaFromFlash(
                    (uint8_t*)rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                    (rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000) & 0xFFFF,
                    (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                    );
            }
        }
        else if (rcp->pi.reg[PI_CART_ADDR_REG] >= 0x06000000
            && rcp->pi.reg[PI_CART_ADDR_REG] < 0x08000000)
        {
        }
        else
        {
            LOG_WARNING("%s: Unknown dma write 0x%x", __FUNCTION__, (int32_t)rcp->pi.reg[PI_CART_ADDR_REG]);
        }

        rcp->pi.reg[PI_STATUS_REG] |= 1;
        cpu->getCp0()->updateCount(*PC);
        interrupt->addInterruptEvent(PI_INT, /*rcp->pi.register.pi_wr_len_reg*/0x1000);

        return;
    }

    if (rcp->pi.reg[PI_CART_ADDR_REG] >= 0x1fc00000) // for paper mario
    {
        rcp->pi.reg[PI_STATUS_REG] |= 1;
        cpu->getCp0()->updateCount(*PC);
        interrupt->addInterruptEvent(PI_INT, 0x1000);

        return;
    }

    uint32_t longueur = (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1;
    int32_t i = (rcp->pi.reg[PI_CART_ADDR_REG] - 0x10000000) & 0x3FFFFFF;
    longueur = (i + (int32_t)longueur) > rom->getSize() ?
        (rom->getSize() - i) : longueur;

    longueur = (rcp->pi.reg[PI_DRAM_ADDR_REG] + longueur) > 0x7FFFFF ?
        (0x7FFFFF - rcp->pi.reg[PI_DRAM_ADDR_REG]) : longueur;

    if (i > rom->getSize() || rcp->pi.reg[PI_DRAM_ADDR_REG] > 0x7FFFFF)
    {
        rcp->pi.reg[PI_STATUS_REG] |= 3;
        cpu->getCp0()->updateCount(*PC);
        interrupt->addInterruptEvent(PI_INT, longueur / 8);

        return;
    }

    vec_for(i = 0; i < (int32_t)longueur; i++)
    {
        ((uint8_t*)rdram->mem)[BES(rcp->pi.reg[PI_DRAM_ADDR_REG] + i)] =
            rom->getImage()[BES(((rcp->pi.reg[PI_CART_ADDR_REG] - 0x10000000) & 0x3FFFFFF) + i)];
    }

    // Set the RDRAM memory size when copying main ROM code
    // (This is just a convenient way to run this code once at the beginning)
    if (rcp->pi.reg[PI_CART_ADDR_REG] == 0x10001000)
    {
        switch (rom->getCICChip())
        {
        case 1:
        case 2:
        case 3:
        case 6:
            rdram->mem[0x318 / 4] = 0x800000;
            break;
        case 5:
            rdram->mem[0x3F0 / 4] = 0x800000;
            break;
        }
    }

    rcp->pi.reg[PI_STATUS_REG] |= 3;
    cpu->getCp0()->updateCount(*PC);
    interrupt->addInterruptEvent(PI_INT, longueur / 8);
}

void DMA::readSI(void)
{
    if (rcp->si.reg[SI_PIF_ADDR_RD64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR("%s: unknown SI use", __FUNCTION__);
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

void DMA::writeSI(void)
{
    if (rcp->si.reg[SI_PIF_ADDR_WR64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR("%s: unknown SI use", __FUNCTION__);
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

