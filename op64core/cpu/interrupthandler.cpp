#include <oplog.h>

#include "icpu.h"
#include "interrupthandler.h"
#include "cp0.h"

#include <core/systiming.h>
#include <cheat/cheatengine.h>
#include <mem/imemory.h>
#include <pif/pif.h>
#include <plugin/plugincontainer.h>
#include <plugin/gfxplugin.h>
#include <rcp/rcpcommon.h>
#include <rcp/rcp.h>
#include <rom/rom.h>
#include <ui/corecontrol.h>


static bool _SPECIAL_done = false;

bool Interrupt::operator<(const Interrupt& i) const
{
    if (this->count - Bus::state.cp0_reg[CP0_COUNT_REG] < 0x80000000)
    {
        if (i.count - Bus::state.cp0_reg[CP0_COUNT_REG] < 0x80000000)
        {
            if ((this->count - Bus::state.cp0_reg[CP0_COUNT_REG]) < (i.count - Bus::state.cp0_reg[CP0_COUNT_REG]))
                return true;

            return false;
        }
        else
        {
            if ((Bus::state.cp0_reg[CP0_COUNT_REG] - i.count) < 0x10000000)
            {
                switch (i.type)
                {
                case SPECIAL_INT:
                    if (_SPECIAL_done)
                        return true;
                    else
                        return false;
                    break;
                default:
                    return false;
                }
            }
            
            return true;
        }
    }

    return false;
}

bool Interrupt::operator==(const Interrupt& i) const
{
    if (this->type == i.type)
        return true;

    return false;
}

InterruptHandler::InterruptHandler(void)
{
}

InterruptHandler::~InterruptHandler(void)
{
}

void InterruptHandler::initialize(Bus* bus)
{
    _bus = bus;

    Bus::state.interrupt_unsafe_state = false;
    _SPECIAL_done = true;
    Bus::state.next_vi = Bus::state.next_interrupt = 5000;
    Bus::state.vi_delay = Bus::state.next_vi;
    Bus::state.vi_field = 0;

    // reset the queue
    q.clear();

    addInterruptEventCount(VI_INT, Bus::state.next_vi);
    addInterruptEventCount(SPECIAL_INT, 0);
}

void InterruptHandler::addInterruptEvent(int32_t type, uint32_t delay)
{
    if (findEvent(type)) {
        LOG_WARNING(InterruptHandler) << "Two events of type 0x" << std::hex << type << " in interrupt queue";
    }

    uint32_t count = Bus::state.cp0_reg[CP0_COUNT_REG] + delay;
    bool special = (type == SPECIAL_INT);

    if (Bus::state.cp0_reg[CP0_COUNT_REG] > 0x80000000)
        _SPECIAL_done = false;

    Interrupt event(type, count);

    auto iter = q.begin();

    for (; iter != q.end(); iter++)
    {
        if (event < *iter && !special)
            break;
    }

    if (iter != q.end() && type != SPECIAL_INT)
    {
        while (iter != q.end() && (*iter).count == count)
            iter++;
    }
    
    q.insert(iter, event);
    Bus::state.next_interrupt = q.front().count;
}

void InterruptHandler::addInterruptEventCount(int32_t type, uint32_t count)
{
    addInterruptEvent(type, (count - Bus::state.cp0_reg[CP0_COUNT_REG]));
}

void InterruptHandler::generateInterrupt(void)
{
    if (CoreControl::stop == true)
    {
        _vi_counter = 0;
    }

    if (!Bus::state.interrupt_unsafe_state)
    {
        if (CoreControl::doHardReset)
        {
            doHardReset();
            CoreControl::doHardReset = false;
            return;
        }
    }

    Interrupt& top = q.front();

    if (Bus::state.skip_jump)
    {
        uint32_t dest = Bus::state.skip_jump;
        Bus::state.skip_jump = 0;

        if (top.count > Bus::state.cp0_reg[CP0_COUNT_REG] || (Bus::state.cp0_reg[CP0_COUNT_REG] - top.count) < 0x80000000)
        {
            Bus::state.next_interrupt = top.count;
        }
        else
        {
            Bus::state.next_interrupt = 0;
        }

        Bus::state.last_jump_addr = dest;
        _bus->cpu->globalJump(dest);
        return;
    }

    switch (top.type)
    {
    case SPECIAL_INT:
    {
        if (Bus::state.cp0_reg[CP0_COUNT_REG] > 0x10000000)
            return;

        popInterruptEvent();
        addInterruptEventCount(SPECIAL_INT, 0);
        return;
    }
        break;
    case VI_INT:
    {
        if (_vi_counter < 60)
        {
            if (_vi_counter == 0)
            {
                _bus->cheat->applyCheats(ENTRY_BOOT);
            }
            _vi_counter++;
        }
        else
        {
            _bus->cheat->applyCheats(ENTRY_VI);
        }

        _bus->plugins->gfx()->UpdateScreen();

        _bus->systimer->doVILimit();

        Bus::state.vi_field ^= (Bus::rcp.vi.reg[VI_STATUS_REG] >> 6) & 0x1;

        Bus::state.vi_delay = (Bus::rcp.vi.reg[VI_V_SYNC_REG] == 0) ? 500000 : (Bus::rcp.vi.reg[VI_V_SYNC_REG] + 1) * CoreControl::VIRefreshRate;

        Bus::state.next_vi += Bus::state.vi_delay;

        popInterruptEvent();
        addInterruptEventCount(VI_INT, Bus::state.next_vi);

        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x08;

        if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case COMPARE_INT:
    {
        popInterruptEvent();
        Bus::state.cp0_reg[CP0_COUNT_REG] += _bus->rom->getCountPerOp();
        addInterruptEventCount(COMPARE_INT, Bus::state.cp0_reg[CP0_COMPARE_REG]);
        Bus::state.cp0_reg[CP0_COUNT_REG] -= _bus->rom->getCountPerOp();

        Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x8000) & 0xFFFFFF83;
        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case CHECK_INT:
        popInterruptEvent();
        break;
    case SI_INT:
    {
        _bus->pif->ram[0x3F] = 0x0;
        popInterruptEvent();
        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x02;
        Bus::rcp.si.reg[SI_STATUS_REG] |= 0x1000;

        if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case PI_INT:
    {
        popInterruptEvent();
        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x10;
        Bus::rcp.pi.reg[PI_STATUS_REG] &= ~3;

        if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case AI_INT:
    {
        if (Bus::rcp.ai.reg[AI_STATUS_REG] & 0x80000000) // full
        {
            uint32_t ai_event = findEvent(AI_INT);
            popInterruptEvent();
            Bus::rcp.ai.reg[AI_STATUS_REG] &= ~0x80000000;
            Bus::rcp.ai.fifo[0].delay = Bus::rcp.ai.fifo[1].delay;
            Bus::rcp.ai.fifo[0].length = Bus::rcp.ai.fifo[1].length;
            addInterruptEventCount(AI_INT, ai_event + Bus::rcp.ai.fifo[1].delay);

            Bus::rcp.mi.reg[MI_INTR_REG] |= 0x04;

            if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
            {
                Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
            }
            else
            {
                return;
            }

            if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
                return;

            if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
                return;
        }
        else
        {
            popInterruptEvent();
            Bus::rcp.ai.reg[AI_STATUS_REG] &= ~0x40000000;

            Bus::rcp.mi.reg[MI_INTR_REG] |= 0x04;

            if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
            {
                Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
            }
            else
            {
                return;
            }

            if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
                return;

            if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
                return;
        }
    }
        break;
    case SP_INT:
    {
        popInterruptEvent();
        Bus::rcp.sp.reg[SP_STATUS_REG] |= 0x203;
        // sp_register.sp_status_reg |= 0x303;

        if (!(Bus::rcp.sp.reg[SP_STATUS_REG] & 0x40))
            return; // !intr_on_break

        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x01;

        if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case DP_INT:
    {
        popInterruptEvent();
        Bus::rcp.dpc.reg[DPC_STATUS_REG] &= ~2;
        Bus::rcp.dpc.reg[DPC_STATUS_REG] |= 0x81;
        Bus::rcp.mi.reg[MI_INTR_REG] |= 0x20;

        if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case HW2_INT:
    {
        popInterruptEvent();

        Bus::state.cp0_reg[CP0_STATUS_REG] = (Bus::state.cp0_reg[CP0_STATUS_REG] & ~0x00380000) | 0x1000;
        Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x1000) & 0xFFFFFF83;
    }
        break;
    case NMI_INT:
    {
        popInterruptEvent();
        // setup r4300 Status flags: reset TS and SR, set BEV, ERL, and SR
        Bus::state.cp0_reg[CP0_STATUS_REG] = (Bus::state.cp0_reg[CP0_STATUS_REG] & ~0x00380000) | 0x00500004;
        Bus::state.cp0_reg[CP0_CAUSE_REG] = 0x00000000;
        // simulate the soft reset code which would run from the PIF ROM
        _bus->cpu->softReset();
        // clear all interrupts, reset interrupt counters back to 0
        Bus::state.cp0_reg[CP0_COUNT_REG] = 0;
        _vi_counter = 0;
        initialize(_bus);
        // clear the audio status register so that subsequent write_ai() calls will work properly
        Bus::rcp.ai.reg[AI_STATUS_REG] = 0;
        // set ErrorEPC with the last instruction address
        Bus::state.cp0_reg[CP0_ERROREPC_REG] = (uint32_t) Bus::state.PC;
        
        // adjust ErrorEPC if we were in a delay slot, and clear the delay_slot and dyna_interp flags
        if (_bus->cpu->inDelaySlot())
        {
            Bus::state.cp0_reg[CP0_ERROREPC_REG] -= 4;
        }

        _bus->cpu->setDelaySlot(false);

        // set next instruction address to reset vector
        Bus::state.last_jump_addr = 0xa4000040;
        _bus->cpu->globalJump(0xa4000040);
        return;
    }
        break;
    default:
    {
        LOG_WARNING(InterruptHandler) << "Unknown interrupt queue event type " << std::hex << top.type;
        popInterruptEvent();
    }
        break;
    }

    _bus->cpu->generalException();

    // TODO future savestate
}

void InterruptHandler::doHardReset(void)
{
    _bus->mem->initialize(_bus);
    _bus->cpu->hardReset();
    _bus->cpu->softReset();

    if (!_bus->plugins->initialize(_bus))
    {
        LOG_ERROR(InterruptHandler) << "One or more plugins failed to initialize";
        CoreControl::stop = true;
        return;
    }

    Bus::state.last_jump_addr = 0xa4000040;
    Bus::state.next_interrupt = 624999;
    initialize(_bus);
    _bus->cpu->globalJump(Bus::state.last_jump_addr);
}

void InterruptHandler::popInterruptEvent(void)
{
    if (q.front().type == SPECIAL_INT)
        _SPECIAL_done = true;

    q.pop_front();

    if (!q.empty() && (q.front().count > Bus::state.cp0_reg[CP0_COUNT_REG] || (Bus::state.cp0_reg[CP0_COUNT_REG] - q.front().count) < 0x80000000))
    {
        Bus::state.next_interrupt = q.front().count;
    }
    else
    {
        Bus::state.next_interrupt = 0;
    }
}

uint32_t InterruptHandler::findEvent(int32_t type)
{
    auto event = std::find(q.begin(), q.end(), Interrupt(type));
    if (event != q.end())
    {
        return (*event).count;
    }

    return 0;
}

void InterruptHandler::checkInterrupt(void)
{
    if (Bus::rcp.mi.reg[MI_INTR_REG] & Bus::rcp.mi.reg[MI_INTR_MASK_REG])
    {
        Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
    }
    else
    {
        Bus::state.cp0_reg[CP0_CAUSE_REG] &= ~0x400;
    }

    if ((Bus::state.cp0_reg[CP0_STATUS_REG] & 7) != 1)
        return;

    if (Bus::state.cp0_reg[CP0_STATUS_REG] & Bus::state.cp0_reg[CP0_CAUSE_REG] & 0xFF00)
    {
        // Overrides the sort and make this the next interrupt
        q.push_front(Interrupt(CHECK_INT, Bus::state.cp0_reg[CP0_COUNT_REG]));
        Bus::state.next_interrupt = Bus::state.cp0_reg[CP0_COUNT_REG];
    }
}

void InterruptHandler::deleteEvent(int32_t type)
{
    if (q.empty())
        return;

    auto event = std::find(q.begin(), q.end(), Interrupt(type));
    if (event != q.end())
    {
        q.erase(event);
    }
}

void InterruptHandler::translateEventQueueBy(uint32_t base)
{
    deleteEvent(COMPARE_INT);
    deleteEvent(SPECIAL_INT);

    std::for_each(q.begin(), q.end(), [&](Interrupt& i){
        i.count = (i.count - Bus::state.cp0_reg[CP0_COUNT_REG]) + base;
    });

    addInterruptEventCount(COMPARE_INT, Bus::state.cp0_reg[CP0_COMPARE_REG]);
    addInterruptEventCount(SPECIAL_INT, 0);
}

void InterruptHandler::softReset(void)
{
    addInterruptEvent(HW2_INT, 0);  /* Hardware 2 Interrupt immediately */
    addInterruptEvent(NMI_INT, 50000000);  /* Non maskable Interrupt after 1/2 second */
}
