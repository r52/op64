#include <oputil.h>
#include <oplog.h>

#include "peripheralinterface.h"

#include <core/bus.h>
#include <rcp/rcp.h>
#include <cpu/icpu.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>
#include <rom/rom.h>
#include <rom/flashram.h>
#include <rom/sram.h>


void PeripheralInterface::DMARead(void)
{
    using namespace Bus;
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
                (uint8_t*) rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000,
                (rcp->pi.reg[PI_RD_LEN_REG] & 0xFFFFFF) + 1
                );
        }

        if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM)
        {
            flashram->dmaToFlash(
                (uint8_t*) rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000,
                (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                );
        }
    }
    else
    {
        LOG_WARNING(PeripheralInterface) << "Unknown dma read";
    }

    rcp->pi.reg[PI_STATUS_REG] |= 1;
    Bus::cpu->getCp0()->updateCount(*PC);
    Bus::interrupt->addInterruptEvent(PI_INT, 0x1000/*rcp->pi.register.pi_rd_len_reg*/);
}

void PeripheralInterface::DMAWrite(void)
{
    using namespace Bus;
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
                    (uint8_t*) rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
                    (rcp->pi.reg[PI_CART_ADDR_REG] - 0x08000000) & 0xFFFF,
                    (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1
                    );
            }

            if (rom->getSaveType() == SAVETYPE_FLASH_RAM)
            {
                flashram->dmaFromFlash(
                    (uint8_t*) rdram->mem + rcp->pi.reg[PI_DRAM_ADDR_REG],
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
            LOG_WARNING(PeripheralInterface) << "Unknown dma write 0x" << std::hex << (int32_t) rcp->pi.reg[PI_CART_ADDR_REG];
        }

        rcp->pi.reg[PI_STATUS_REG] |= 1;
        Bus::cpu->getCp0()->updateCount(*PC);
        Bus::interrupt->addInterruptEvent(PI_INT, /*rcp->pi.register.pi_wr_len_reg*/0x1000);

        return;
    }

    if (rcp->pi.reg[PI_CART_ADDR_REG] >= 0x1fc00000) // for paper mario
    {
        rcp->pi.reg[PI_STATUS_REG] |= 1;
        Bus::cpu->getCp0()->updateCount(*PC);
        Bus::interrupt->addInterruptEvent(PI_INT, 0x1000);

        return;
    }

    uint32_t longueur = (rcp->pi.reg[PI_WR_LEN_REG] & 0xFFFFFF) + 1;
    uint32_t i = (rcp->pi.reg[PI_CART_ADDR_REG] - 0x10000000) & 0x3FFFFFF;
    longueur = (i + (int32_t)longueur) > rom->getSize() ?
        (rom->getSize() - i) : longueur;

    longueur = (rcp->pi.reg[PI_DRAM_ADDR_REG] + longueur) > 0x7FFFFF ?
        (0x7FFFFF - rcp->pi.reg[PI_DRAM_ADDR_REG]) : longueur;

    if (i > rom->getSize() || rcp->pi.reg[PI_DRAM_ADDR_REG] > 0x7FFFFF)
    {
        rcp->pi.reg[PI_STATUS_REG] |= 3;
        Bus::cpu->getCp0()->updateCount(*PC);
        Bus::interrupt->addInterruptEvent(PI_INT, longueur / 8);

        return;
    }

    for(i = 0; i < longueur; i++)
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
    Bus::cpu->getCp0()->updateCount(*PC);
    Bus::interrupt->addInterruptEvent(PI_INT, longueur / 8);
}

OPStatus PeripheralInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[PI_REG(address)];
    return OP_OK;
}

OPStatus PeripheralInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = PI_REG(address);

    switch (regnum)
    {
    case PI_RD_LEN_REG:
        masked_write(&reg[PI_RD_LEN_REG], data, mask);
        DMARead();
        return OP_OK;

    case PI_WR_LEN_REG:
        masked_write(&reg[PI_WR_LEN_REG], data, mask);
        DMAWrite();
        return OP_OK;

    case PI_STATUS_REG:
        if (data & mask & 2)
        {
            Bus::rcp->mi.reg[MI_INTR_REG] &= ~0x10;
            Bus::interrupt->checkInterrupt();
        }

        return OP_OK;

    case PI_BSD_DOM1_LAT_REG:
    case PI_BSD_DOM1_PWD_REG:
    case PI_BSD_DOM1_PGS_REG:
    case PI_BSD_DOM1_RLS_REG:
    case PI_BSD_DOM2_LAT_REG:
    case PI_BSD_DOM2_PWD_REG:
    case PI_BSD_DOM2_PGS_REG:
    case PI_BSD_DOM2_RLS_REG:
        masked_write(&reg[regnum], data & 0xff, mask);
        return OP_OK;
    }

    masked_write(&reg[regnum], data, mask);
    
    return OP_OK;
}
