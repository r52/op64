#include "interrupthandler.h"
#include "cp0.h"
#include "imemory.h"
#include "icpu.h"
#include "logger.h"
#include "rcpcommon.h"
#include "rom.h"
#include "plugins.h"
#include "gfxplugin.h"
#include "systiming.h"
#include "corecontrol.h"

bool Interrupt::operator<(const Interrupt& i) const
{
    using namespace Bus;

    if (this->count - cp0_reg[CP0_COUNT_REG] < 0x80000000)
    {
        if (i.count - cp0_reg[CP0_COUNT_REG] < 0x80000000)
        {
            if ((this->count - cp0_reg[CP0_COUNT_REG]) < (i.count - cp0_reg[CP0_COUNT_REG]))
                return true;

            return false;
        }
        else
        {
            if ((cp0_reg[CP0_COUNT_REG] - i.count) < 0x10000000)
            {
                if (i.type == SPECIAL_INT && SPECIAL_done)
                    return true;

                return false;
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
    using namespace Bus;
    next_vi = &_next_vi;
    vi_field = &_vi_field;
    next_interrupt = &_next_interrupt;
    SPECIAL_done = &_SPECIAL_done;
    perform_hard_reset = &_perform_hard_reset;
    interrupt_unsafe_state = &_interrupt_unsafe_state;
}

InterruptHandler::~InterruptHandler(void)
{
    using namespace Bus;
    next_vi = nullptr;
    vi_field = nullptr;
    next_interrupt = nullptr;
    SPECIAL_done = nullptr;
    perform_hard_reset = nullptr;
    interrupt_unsafe_state = nullptr;
}

void InterruptHandler::initialize(void)
{
    _SPECIAL_done = true;
    _next_vi = _next_interrupt = 5000;
    Bus::vi_reg[_VI_DELAY] = _next_vi;
    _vi_field = 0;

    // reset the queue
    q.clear();

    add_interrupt_event_count(VI_INT, _next_vi);
    add_interrupt_event_count(SPECIAL_INT, 0);
}

void InterruptHandler::add_interrupt_event(int32_t type, uint32_t delay)
{
    if (find_event(type)) {
        LOG_WARNING("two events of type 0x%x in interrupt queue\n", type);
    }

    uint32_t count = Bus::cp0_reg[CP0_COUNT_REG] + delay;
    bool special = (type == SPECIAL_INT);

    if (Bus::cp0_reg[CP0_COUNT_REG] > 0x80000000)
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
    _next_interrupt = q.front().count;
}

void InterruptHandler::add_interrupt_event_count(int32_t type, uint32_t count)
{
    add_interrupt_event(type, (count - Bus::cp0_reg[CP0_COUNT_REG]));
}

void InterruptHandler::gen_interrupt(void)
{
    if (Bus::stop == true)
    {
        _vi_counter = 0;
    }

    if (!_interrupt_unsafe_state)
    {
        if (_perform_hard_reset)
        {
            do_hard_reset();
            _perform_hard_reset = false;
            return;
        }
    }

    Interrupt& top = q.front();

    if (*(Bus::skip_jump))
    {
        uint32_t dest = *(Bus::skip_jump);
        *(Bus::skip_jump) = 0;

        if (top.count > Bus::cp0_reg[CP0_COUNT_REG] || (Bus::cp0_reg[CP0_COUNT_REG] - top.count) < 0x80000000)
        {
            _next_interrupt = top.count;
        }
        else
        {
            _next_interrupt = 0;
        }

        *(Bus::last_instr_addr) = dest;
        Bus::cpu->global_jump_to(dest);
        return;
    }

    switch (top.type)
    {
    case SPECIAL_INT:
    {
        if (Bus::cp0_reg[CP0_COUNT_REG] > 0x10000000)
            return;

        pop_interrupt_event();
        add_interrupt_event_count(SPECIAL_INT, 0);
        return;
    }
        break;
    case VI_INT:
    {
        if (_vi_counter < 60)
            _vi_counter++;

        // TODO future: cheats here???

        Bus::plugins->gfx()->UpdateScreen();

        uint64_t frameRate;
        if (frameRate = Bus::systimer->doVILimit())
        {
            if (CoreControl::displayVI)
            {
                CoreControl::displayVI(frameRate);
            }
        }

        if (Bus::vi_reg[VI_V_SYNC_REG] == 0)
            Bus::vi_reg[_VI_DELAY] = 500000;
        else
            Bus::vi_reg[_VI_DELAY] = ((Bus::vi_reg[VI_V_SYNC_REG] + 1) * 1500);

        _next_vi += Bus::vi_reg[_VI_DELAY];

        if (Bus::vi_reg[VI_STATUS_REG] & 0x40)
            _vi_field = 1 - _vi_field;
        else 
            _vi_field = 0;

        pop_interrupt_event();
        add_interrupt_event_count(VI_INT, _next_vi);

        Bus::mi_reg[MI_INTR_REG] |= 0x08;

        if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
        {
            Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case COMPARE_INT:
    {
        pop_interrupt_event();
        Bus::cp0_reg[CP0_COUNT_REG] += Bus::rom->getCountPerOp();
        add_interrupt_event_count(COMPARE_INT, Bus::cp0_reg[CP0_COMPARE_REG]);
        Bus::cp0_reg[CP0_COUNT_REG] -= Bus::rom->getCountPerOp();

        Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x8000) & 0xFFFFFF83;
        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case CHECK_INT:
        pop_interrupt_event();
        break;
    case SI_INT:
    {
        Bus::pif_ram8[0x3F] = 0x0;
        pop_interrupt_event();
        Bus::mi_reg[MI_INTR_REG] |= 0x02;
        Bus::si_reg[SI_STATUS_REG] |= 0x1000;

        if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
        {
            Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case PI_INT:
    {
        pop_interrupt_event();
        Bus::mi_reg[MI_INTR_REG] |= 0x10;
        Bus::pi_reg[PI_STATUS_REG] &= ~3;

        if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
        {
            Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case AI_INT:
    {
        if (Bus::ai_reg[AI_STATUS_REG] & 0x80000000) // full
        {
            uint32_t ai_event = find_event(AI_INT);
            pop_interrupt_event();
            Bus::ai_reg[AI_STATUS_REG] &= ~0x80000000;
            Bus::ai_reg[_AI_CURRENT_DELAY] = Bus::ai_reg[_AI_NEXT_DELAY];
            Bus::ai_reg[_AI_CURRENT_LEN] = Bus::ai_reg[_AI_NEXT_LEN];
            add_interrupt_event_count(AI_INT, ai_event + Bus::ai_reg[_AI_NEXT_DELAY]);

            Bus::mi_reg[MI_INTR_REG] |= 0x04;

            if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
            {
                Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
            }
            else
            {
                return;
            }

            if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
                return;

            if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
                return;
        }
        else
        {
            pop_interrupt_event();
            Bus::ai_reg[AI_STATUS_REG] &= ~0x40000000;

            Bus::mi_reg[MI_INTR_REG] |= 0x04;

            if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
            {
                Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
            }
            else
            {
                return;
            }

            if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
                return;

            if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
                return;
        }
    }
        break;
    case SP_INT:
    {
        pop_interrupt_event();
        Bus::sp_reg[SP_STATUS_REG] |= 0x203;
        // sp_register.sp_status_reg |= 0x303;

        if (!(Bus::sp_reg[SP_STATUS_REG] & 0x40))
            return; // !intr_on_break

        Bus::mi_reg[MI_INTR_REG] |= 0x01;

        if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
        {
            Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case DP_INT:
    {
        pop_interrupt_event();
        Bus::dp_reg[DPC_STATUS_REG] &= ~2;
        Bus::dp_reg[DPC_STATUS_REG] |= 0x81;
        Bus::mi_reg[MI_INTR_REG] |= 0x20;

        if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
        {
            Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
        }
        else
        {
            return;
        }

        if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
            return;

        if (!(Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00))
            return;
    }
        break;
    case HW2_INT:
    {
        pop_interrupt_event();

        Bus::cp0_reg[CP0_STATUS_REG] = (Bus::cp0_reg[CP0_STATUS_REG] & ~0x00380000) | 0x1000;
        Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x1000) & 0xFFFFFF83;
    }
        break;
    case NMI_INT:
    {
        pop_interrupt_event();
        // setup r4300 Status flags: reset TS and SR, set BEV, ERL, and SR
        Bus::cp0_reg[CP0_STATUS_REG] = (Bus::cp0_reg[CP0_STATUS_REG] & ~0x00380000) | 0x00500004;
        Bus::cp0_reg[CP0_CAUSE_REG] = 0x00000000;
        // simulate the soft reset code which would run from the PIF ROM
        Bus::cpu->soft_reset();
        // clear all interrupts, reset interrupt counters back to 0
        Bus::cp0_reg[CP0_COUNT_REG] = 0;
        _vi_counter = 0;
        initialize();
        // clear the audio status register so that subsequent write_ai() calls will work properly
        Bus::ai_reg[AI_STATUS_REG] = 0;
        // set ErrorEPC with the last instruction address
        Bus::cp0_reg[CP0_ERROREPC_REG] = (uint32_t)*(Bus::PC);
        
        // adjust ErrorEPC if we were in a delay slot, and clear the delay_slot and dyna_interp flags
        if (*(Bus::delay_slot))
        {
            Bus::cp0_reg[CP0_ERROREPC_REG] -= 4;
        }

        *(Bus::delay_slot) = false;

        // set next instruction address to reset vector
        *(Bus::last_instr_addr) = 0xa4000040;
        Bus::cpu->global_jump_to(0xa4000040);
        return;
    }
        break;
    default:
    {
        LOG_DEBUG("ERROR: Unknown interrupt queue event type %.8X.\n", top.type);
        pop_interrupt_event();
    }
        break;
    }

    Bus::cpu->general_exception();

    // TODO future savestate
}

void InterruptHandler::do_hard_reset(void)
{
    Bus::mem->initialize();
    Bus::cpu->hard_reset();
    Bus::cpu->soft_reset();
    *(Bus::last_instr_addr) = 0xa4000040;
    _next_interrupt = 624999;
    initialize();
    Bus::cpu->global_jump_to(*(Bus::last_instr_addr));
}

void InterruptHandler::pop_interrupt_event(void)
{
    if (q.front().type == SPECIAL_INT)
        _SPECIAL_done = true;

    q.pop_front();

    if (!q.empty() && (q.front().count > Bus::cp0_reg[CP0_COUNT_REG] || (Bus::cp0_reg[CP0_COUNT_REG] - q.front().count) < 0x80000000))
    {
        _next_interrupt = q.front().count;
    }
    else
    {
        _next_interrupt = 0;
    }
}

uint32_t InterruptHandler::find_event(int32_t type)
{
    auto event = std::find(q.begin(), q.end(), Interrupt(type));
    if (event != q.end())
    {
        return (*event).count;
    }

    return 0;
}

void InterruptHandler::check_interrupt(void)
{
    if (Bus::mi_reg[MI_INTR_REG] & Bus::mi_reg[MI_INTR_MASK_REG])
    {
        Bus::cp0_reg[CP0_CAUSE_REG] = (Bus::cp0_reg[CP0_CAUSE_REG] | 0x400) & 0xFFFFFF83;
    }
    else
    {
        Bus::cp0_reg[CP0_CAUSE_REG] &= ~0x400;
    }

    if ((Bus::cp0_reg[CP0_STATUS_REG] & 7) != 1)
        return;

    if (Bus::cp0_reg[CP0_STATUS_REG] & Bus::cp0_reg[CP0_CAUSE_REG] & 0xFF00)
    {
        // Overrides the sort and make this the next interrupt
        q.push_front(Interrupt(CHECK_INT, Bus::cp0_reg[CP0_COUNT_REG]));
        _next_interrupt = Bus::cp0_reg[CP0_COUNT_REG];
    }
}

void InterruptHandler::delete_event(int32_t type)
{
    if (q.empty())
        return;

    auto event = std::find(q.begin(), q.end(), Interrupt(type));
    if (event != q.end())
    {
        q.erase(event);
    }
}

void InterruptHandler::translate_event_queue_by(uint32_t base)
{
    delete_event(COMPARE_INT);
    delete_event(SPECIAL_INT);

    std::for_each(q.begin(), q.end(), [&](Interrupt& i){
        i.count = (i.count - Bus::cp0_reg[CP0_COUNT_REG]) + base;
    });

    add_interrupt_event_count(COMPARE_INT, Bus::cp0_reg[CP0_COMPARE_REG]);
    add_interrupt_event_count(SPECIAL_INT, 0);
}
