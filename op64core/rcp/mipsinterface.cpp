#include "mipsinterface.h"

#include <core/bus.h>
#include <cpu/cp0.h>
#include <cpu/icpu.h>
#include <cpu/interrupthandler.h>

OPStatus MIPSInterface::read(uint32_t address, uint32_t* data)
{
    *data = reg[MI_REG(address)];

    return OP_OK;
}

OPStatus MIPSInterface::write(uint32_t address, uint32_t data, uint32_t mask)
{
    uint32_t regnum = MI_REG(address);

    switch (regnum)
    {
    case MI_INIT_MODE_REG:
        if (update_mi_init_mode(data & mask))
        {
            /* clear DP interrupt */
            reg[MI_INTR_REG] &= ~0x20;
            Bus::interrupt->checkInterrupt();
        }
        break;
    case MI_INTR_MASK_REG:
        update_mi_intr_mask(data & mask);

        Bus::interrupt->checkInterrupt();
        Bus::cpu->getCp0()->updateCount(*Bus::PC);

        if (Bus::next_interrupt <= Bus::cp0_reg[CP0_COUNT_REG])
            Bus::interrupt->generateInterrupt();

        break;
    }

    return OP_OK;
}

bool MIPSInterface::update_mi_init_mode(uint32_t w)
{
    bool clear_dp = false;

    reg[MI_INIT_MODE_REG] &= ~0x7F; // init_length
    reg[MI_INIT_MODE_REG] |= w & 0x7F;

    if (w & 0x80) // clear init_mode
        reg[MI_INIT_MODE_REG] &= ~0x80;
    if (w & 0x100) // set init_mode
        reg[MI_INIT_MODE_REG] |= 0x80;

    if (w & 0x200) // clear ebus_test_mode
        reg[MI_INIT_MODE_REG] &= ~0x100;
    if (w & 0x400) // set ebus_test_mode
        reg[MI_INIT_MODE_REG] |= 0x100;

    if (w & 0x800) // clear DP interrupt
    {
        clear_dp = true;
    }

    if (w & 0x1000) // clear RDRAM_reg_mode
        reg[MI_INIT_MODE_REG] &= ~0x200;
    if (w & 0x2000) // set RDRAM_reg_mode
        reg[MI_INIT_MODE_REG] |= 0x200;

    return clear_dp;
}

void MIPSInterface::update_mi_intr_mask(uint32_t w)
{
    if (w & 0x1)   reg[MI_INTR_MASK_REG] &= ~0x1; // clear SP mask
    if (w & 0x2)   reg[MI_INTR_MASK_REG] |= 0x1; // set SP mask
    if (w & 0x4)   reg[MI_INTR_MASK_REG] &= ~0x2; // clear SI mask
    if (w & 0x8)   reg[MI_INTR_MASK_REG] |= 0x2; // set SI mask
    if (w & 0x10)  reg[MI_INTR_MASK_REG] &= ~0x4; // clear AI mask
    if (w & 0x20)  reg[MI_INTR_MASK_REG] |= 0x4; // set AI mask
    if (w & 0x40)  reg[MI_INTR_MASK_REG] &= ~0x8; // clear VI mask
    if (w & 0x80)  reg[MI_INTR_MASK_REG] |= 0x8; // set VI mask
    if (w & 0x100) reg[MI_INTR_MASK_REG] &= ~0x10; // clear PI mask
    if (w & 0x200) reg[MI_INTR_MASK_REG] |= 0x10; // set PI mask
    if (w & 0x400) reg[MI_INTR_MASK_REG] &= ~0x20; // clear DP mask
    if (w & 0x800) reg[MI_INTR_MASK_REG] |= 0x20; // set DP mask
}
