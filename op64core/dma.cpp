#include "dma.h"

#include "bus.h"
#include "rcpcommon.h"
#include "imemory.h"
#include "logger.h"
#include "icpu.h"
#include "cp0.h"
#include "interrupthandler.h"
#include "rom.h"
#include "configstore.h"
#include "pif.h"
#include "sram.h"
#include "flashram.h"

using namespace Bus;

void DMA::readSP(void)
{
    uint32_t len_reg = sp_reg[SP_WR_LEN_REG];

    uint32_t length = ((len_reg & 0xfff) | 7) + 1;
    uint32_t count = ((len_reg >> 12) & 0xff) + 1;
    uint32_t skip = ((len_reg >> 20) & 0xfff);

    uint32_t memaddr = sp_reg[SP_MEM_ADDR_REG] & 0xfff;
    uint32_t dramaddr = sp_reg[SP_DRAM_ADDR_REG] & 0xffffff;

    uint8_t* spmem = ((sp_reg[SP_MEM_ADDR_REG] & 0x1000) != 0) ? sp_imem8 : sp_dmem8;
    uint8_t* dram = rdram8;

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
    uint32_t len_reg = sp_reg[SP_RD_LEN_REG];

    uint32_t length = ((len_reg & 0xfff) | 7) + 1;
    uint32_t count = ((len_reg >> 12) & 0xff) + 1;
    uint32_t skip = ((len_reg >> 20) & 0xfff);

    uint32_t memaddr = sp_reg[SP_MEM_ADDR_REG] & 0xfff;
    uint32_t dramaddr = sp_reg[SP_DRAM_ADDR_REG] & 0xffffff;

    uint8_t* spmem = ((sp_reg[SP_MEM_ADDR_REG] & 0x1000) != 0) ? sp_imem8 : sp_dmem8;
    uint8_t* dram = rdram8;

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
    if (pi_reg[PI_CART_ADDR_REG] >= 0x08000000
        && pi_reg[PI_CART_ADDR_REG] < 0x08010000)
    {
        if (rom->getSaveType() == SAVETYPE_AUTO)
        {
            rom->setSaveType(SAVETYPE_SRAM);
        }

        if (rom->getSaveType() == SAVETYPE_SRAM)
        {
            sram->dmaToSRAM(
                rdram8 + pi_reg[PI_DRAM_ADDR_REG],
                pi_reg[PI_CART_ADDR_REG] - 0x08000000,
                (pi_reg[PI_RD_LEN_REG] & 0xFFFFFF) + 1
                );
        }

        if (rom->getSaveType() == SAVETYPE_FLASH_RAM)
        {
            flashram->dmaToFlash(
                rdram8 + pi_reg[PI_DRAM_ADDR_REG],
                pi_reg[PI_CART_ADDR_REG] - 0x08000000,
                (pi_reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                );
        }
    }
    else
    {
        LOG_WARNING("%s: Unknown dma read", __FUNCTION__);
    }

    pi_reg[PI_STATUS_REG] |= 1;
    cpu->getcp0()->update_count(*PC);
    interrupt->add_interrupt_event(PI_INT, 0x1000/*pi_register.pi_rd_len_reg*/);
}

void DMA::writePI(void)
{
    if (pi_reg[PI_CART_ADDR_REG] < 0x10000000)
    {
        if (pi_reg[PI_CART_ADDR_REG] >= 0x08000000
            && pi_reg[PI_CART_ADDR_REG] < 0x08010000)
        {
            if (rom->getSaveType() == SAVETYPE_AUTO)
            {
                rom->setSaveType(SAVETYPE_SRAM);
            }

            if (rom->getSaveType() == SAVETYPE_SRAM)
            {
                sram->dmaFromSRAM(
                    rdram8 + pi_reg[PI_DRAM_ADDR_REG],
                    (pi_reg[PI_CART_ADDR_REG] - 0x08000000) & 0xFFFF,
                    (pi_reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                    );
            }

            if (rom->getSaveType() == SAVETYPE_FLASH_RAM)
            {
                flashram->dmaFromFlash(
                    rdram8 + pi_reg[PI_DRAM_ADDR_REG],
                    (pi_reg[PI_CART_ADDR_REG] - 0x08000000) & 0xFFFF,
                    (pi_reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                    );
            }
        }
        else if (pi_reg[PI_CART_ADDR_REG] >= 0x06000000
            && pi_reg[PI_CART_ADDR_REG] < 0x08000000)
        {
        }
        else
        {
            LOG_WARNING("%s: Unknown dma write 0x%x", __FUNCTION__, (int32_t)pi_reg[PI_CART_ADDR_REG]);
        }

        pi_reg[PI_STATUS_REG] |= 1;
        cpu->getcp0()->update_count(*PC);
        interrupt->add_interrupt_event(PI_INT, /*pi_register.pi_wr_len_reg*/0x1000);

        return;
    }

    if (pi_reg[PI_CART_ADDR_REG] >= 0x1fc00000) // for paper mario
    {
        pi_reg[PI_STATUS_REG] |= 1;
        cpu->getcp0()->update_count(*PC);
        interrupt->add_interrupt_event(PI_INT, 0x1000);

        return;
    }

    uint32_t longueur = (pi_reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1;
    int32_t i = (pi_reg[PI_CART_ADDR_REG] - 0x10000000) & 0x3FFFFFF;
    longueur = (i + (int32_t)longueur) > rom->getSize() ?
        (rom->getSize() - i) : longueur;

    longueur = (pi_reg[PI_DRAM_ADDR_REG] + longueur) > 0x7FFFFF ?
        (0x7FFFFF - pi_reg[PI_DRAM_ADDR_REG]) : longueur;

    if (i > rom->getSize() || pi_reg[PI_DRAM_ADDR_REG] > 0x7FFFFF)
    {
        pi_reg[PI_STATUS_REG] |= 3;
        cpu->getcp0()->update_count(*PC);
        interrupt->add_interrupt_event(PI_INT, longueur / 8);

        return;
    }

    vec_for(i = 0; i < (int32_t)longueur; i++)
    {
        rdram8[BES(pi_reg[PI_DRAM_ADDR_REG] + i)] =
            rom_image[BES(((pi_reg[PI_CART_ADDR_REG] - 0x10000000) & 0x3FFFFFF) + i)];
    }

    // Set the RDRAM memory size when copying main ROM code
    // (This is just a convenient way to run this code once at the beginning)
    if (pi_reg[PI_CART_ADDR_REG] == 0x10001000)
    {
        switch (rom->getCICChip())
        {
        case 1:
        case 2:
        case 3:
        case 6:
            rdram[0x318 / 4] = 0x800000;
            break;
        case 5:
            rdram[0x3F0 / 4] = 0x800000;
            break;
        }
    }

    pi_reg[PI_STATUS_REG] |= 3;
    cpu->getcp0()->update_count(*PC);
    interrupt->add_interrupt_event(PI_INT, longueur / 8);
}

void DMA::readSI(void)
{
    if (Bus::si_reg[SI_PIF_ADDR_RD64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR("%s: unknown SI use", __FUNCTION__);
        Bus::stop = true;
    }

    Bus::pif->pifRead();

    for (uint32_t i = 0; i < (64 / 4); i++)
    {
        Bus::rdram[Bus::si_reg[SI_DRAM_ADDR_REG] / 4 + i] = byteswap_u32(Bus::pif_ram32[i]);
    }

    Bus::cpu->getcp0()->update_count(*Bus::PC);

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
    }
    else {
        Bus::mi_reg[MI_INTR_REG] |= 0x02; // SI
        Bus::si_reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        Bus::interrupt->check_interrupt();
    }
}

void DMA::writeSI(void)
{
    if (Bus::si_reg[SI_PIF_ADDR_WR64B_REG] != 0x1FC007C0)
    {
        LOG_ERROR("%s: unknown SI use", __FUNCTION__);
        Bus::stop = true;
    }

    for (uint32_t i = 0; i < (64 / 4); i++)
    {
        Bus::pif_ram32[i] = byteswap_u32(Bus::rdram[Bus::si_reg[SI_DRAM_ADDR_REG] / 4 + i]);
    }

    Bus::pif->pifWrite();
    Bus::cpu->getcp0()->update_count(*Bus::PC);

    if (ConfigStore::getInstance().getBool(CFG_SECTION_CORE, CFG_DELAY_SI)) {
        Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
    }
    else {
        Bus::mi_reg[MI_INTR_REG] |= 0x02; // SI
        Bus::si_reg[SI_STATUS_REG] |= 0x1000; // INTERRUPT
        Bus::interrupt->check_interrupt();
    }
}

