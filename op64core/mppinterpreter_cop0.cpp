#include "mppinterpreter.h"
#include "bus.h"
#include "logger.h"
#include "cp0.h"
#include "cp1.h"
#include "interrupthandler.h"
#include "tlb.h"


void MPPInterpreter::MFC0(void)
{
    switch (_cur_instr.rd)
    {
    case CP0_RANDOM_REG:
        LOG_ERROR("MFC0 instruction reading un-implemented Random register");
        Bus::stop = true;
        break;
    case CP0_COUNT_REG:
        _cp0->update_count(_PC);
    default:
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>(_cp0_reg[_cur_instr.rd]);
        break;
    }
    ++_PC;
}

void MPPInterpreter::MTC0(void)
{
    switch (_cur_instr.rd)
    {
    case CP0_INDEX_REG:
        _cp0_reg[CP0_INDEX_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x8000003F;
        if ((_cp0_reg[CP0_INDEX_REG] & 0x3F) > 31)
        {
            LOG_ERROR("MTC0 instruction writing Index register with TLB index > 31");
            Bus::stop = true;
        }
        break;
    case CP0_RANDOM_REG:
        break;
    case CP0_ENTRYLO0_REG:
        _cp0_reg[CP0_ENTRYLO0_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x3FFFFFFF;
        break;
    case CP0_ENTRYLO1_REG:
        _cp0_reg[CP0_ENTRYLO1_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x3FFFFFFF;
        break;
    case CP0_CONTEXT_REG:
        _cp0_reg[CP0_CONTEXT_REG] = ((uint32_t)_reg[_cur_instr.rt].u & 0xFF800000)
            | (_cp0_reg[CP0_CONTEXT_REG] & 0x007FFFF0);
        break;
    case CP0_PAGEMASK_REG:
        _cp0_reg[CP0_PAGEMASK_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x01FFE000;
        break;
    case CP0_WIRED_REG:
        _cp0_reg[CP0_WIRED_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        _cp0_reg[CP0_RANDOM_REG] = 31;
        break;
    case CP0_BADVADDR_REG:
        break;
    case CP0_COUNT_REG:
        _cp0->update_count(_PC);

        Bus::interrupt_unsafe_state = true;
        if (Bus::next_interrupt <= _cp0_reg[CP0_COUNT_REG])
            Bus::interrupt->gen_interrupt();

        Bus::interrupt_unsafe_state = false;

        Bus::interrupt->translate_event_queue_by((uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF);
        _cp0_reg[CP0_COUNT_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_ENTRYHI_REG:
        _cp0_reg[CP0_ENTRYHI_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFE0FF;
        break;
    case CP0_COMPARE_REG:
        _cp0->update_count(_PC);
        Bus::interrupt->delete_event(COMPARE_INT);
        Bus::interrupt->add_interrupt_event_count(COMPARE_INT, (uint32_t)_reg[_cur_instr.rt].u);
        _cp0_reg[CP0_COMPARE_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        _cp0_reg[CP0_CAUSE_REG] &= 0xFFFF7FFF; //Timer interupt is clear
        break;
    case CP0_STATUS_REG:
        if (((uint32_t)_reg[_cur_instr.rt].u & 0x04000000) != (_cp0_reg[CP0_STATUS_REG] & 0x04000000))
        {
            _cp1->shuffle_fpr_data(_cp0_reg[CP0_STATUS_REG], (uint32_t)_reg[_cur_instr.rt].u);
            _cp1->set_fpr_pointers((uint32_t)_reg[_cur_instr.rt].u);
        }
        _cp0_reg[CP0_STATUS_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        _cp0->update_count(_PC);
        ++_PC;
        Bus::interrupt->check_interrupt();

        Bus::interrupt_unsafe_state = true;
        if (Bus::next_interrupt <= _cp0_reg[CP0_COUNT_REG])
            Bus::interrupt->gen_interrupt();

        Bus::interrupt_unsafe_state = false;
        --_PC;
        break;
    case CP0_CAUSE_REG:
        if ((int64_t)_reg[_cur_instr.rt].s != 0)
        {
            LOG_ERROR("MTC0 instruction trying to write Cause register with non-0 value");
            Bus::stop = true;
        }
        else
        {
            _cp0_reg[CP0_CAUSE_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        }
        break;
    case CP0_EPC_REG:
        _cp0_reg[CP0_EPC_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_PREVID_REG:
        break;
    case CP0_CONFIG_REG:
        _cp0_reg[CP0_CONFIG_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_WATCHLO_REG:
        _cp0_reg[CP0_WATCHLO_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_WATCHHI_REG:
        _cp0_reg[CP0_WATCHHI_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_TAGLO_REG:
        _cp0_reg[CP0_TAGLO_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x0FFFFFC0;
        break;
    case CP0_TAGHI_REG:
        _cp0_reg[CP0_TAGHI_REG] = 0;
        break;
    default:
        LOG_ERROR("Unknown MTC0 write: %d", _cur_instr.fs);
        Bus::stop = true;
        break;
    }

    ++_PC;
}

void MPPInterpreter::TLBR(void)
{
    int index = _cp0_reg[CP0_INDEX_REG] & 0x1F;
    _cp0_reg[CP0_PAGEMASK_REG] = TLB::tlb_entry_table[index].mask << 13;
    _cp0_reg[CP0_ENTRYHI_REG] = ((TLB::tlb_entry_table[index].vpn2 << 13) | TLB::tlb_entry_table[index].asid);
    _cp0_reg[CP0_ENTRYLO0_REG] = (TLB::tlb_entry_table[index].pfn_even << 6) | (TLB::tlb_entry_table[index].c_even << 3)
        | (TLB::tlb_entry_table[index].d_even << 2) | (TLB::tlb_entry_table[index].v_even << 1)
        | TLB::tlb_entry_table[index].g;
    _cp0_reg[CP0_ENTRYLO1_REG] = (TLB::tlb_entry_table[index].pfn_odd << 6) | (TLB::tlb_entry_table[index].c_odd << 3)
        | (TLB::tlb_entry_table[index].d_odd << 2) | (TLB::tlb_entry_table[index].v_odd << 1)
        | TLB::tlb_entry_table[index].g;
    ++_PC;
}

void MPPInterpreter::TLBWI(void)
{
    uint32_t idx = _cp0_reg[CP0_INDEX_REG] & 0x3F;

    TLB::tlb_unmap(&TLB::tlb_entry_table[idx]);

    TLB::tlb_entry_table[idx].g = (_cp0_reg[CP0_ENTRYLO0_REG] & _cp0_reg[CP0_ENTRYLO1_REG] & 1);
    TLB::tlb_entry_table[idx].pfn_even = (_cp0_reg[CP0_ENTRYLO0_REG] & 0x3FFFFFC0) >> 6;
    TLB::tlb_entry_table[idx].pfn_odd = (_cp0_reg[CP0_ENTRYLO1_REG] & 0x3FFFFFC0) >> 6;
    TLB::tlb_entry_table[idx].c_even = (_cp0_reg[CP0_ENTRYLO0_REG] & 0x38) >> 3;
    TLB::tlb_entry_table[idx].c_odd = (_cp0_reg[CP0_ENTRYLO1_REG] & 0x38) >> 3;
    TLB::tlb_entry_table[idx].d_even = (_cp0_reg[CP0_ENTRYLO0_REG] & 0x4) >> 2;
    TLB::tlb_entry_table[idx].d_odd = (_cp0_reg[CP0_ENTRYLO1_REG] & 0x4) >> 2;
    TLB::tlb_entry_table[idx].v_even = (_cp0_reg[CP0_ENTRYLO0_REG] & 0x2) >> 1;
    TLB::tlb_entry_table[idx].v_odd = (_cp0_reg[CP0_ENTRYLO1_REG] & 0x2) >> 1;
    TLB::tlb_entry_table[idx].asid = (_cp0_reg[CP0_ENTRYHI_REG] & 0xFF);
    TLB::tlb_entry_table[idx].vpn2 = (_cp0_reg[CP0_ENTRYHI_REG] & 0xFFFFE000) >> 13;
    //TLB::tlb_entry_table[idx].r = (_cp0_reg[CP0_ENTRYHI_REG] & 0xC000000000000000LL) >> 62;
    TLB::tlb_entry_table[idx].mask = (_cp0_reg[CP0_PAGEMASK_REG] & 0x1FFE000) >> 13;

    TLB::tlb_entry_table[idx].start_even = TLB::tlb_entry_table[idx].vpn2 << 13;
    TLB::tlb_entry_table[idx].end_even = TLB::tlb_entry_table[idx].start_even +
        (TLB::tlb_entry_table[idx].mask << 12) + 0xFFF;
    TLB::tlb_entry_table[idx].phys_even = TLB::tlb_entry_table[idx].pfn_even << 12;


    TLB::tlb_entry_table[idx].start_odd = TLB::tlb_entry_table[idx].end_even + 1;
    TLB::tlb_entry_table[idx].end_odd = TLB::tlb_entry_table[idx].start_odd +
        (TLB::tlb_entry_table[idx].mask << 12) + 0xFFF;
    TLB::tlb_entry_table[idx].phys_odd = TLB::tlb_entry_table[idx].pfn_odd << 12;

    TLB::tlb_map(&TLB::tlb_entry_table[idx]);

    ++_PC;
}

void MPPInterpreter::TLBWR(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::TLBP(void)
{
    _cp0_reg[CP0_INDEX_REG] |= 0x80000000;
    for (uint32_t i = 0; i < 32; i++)
    {
        if (((TLB::tlb_entry_table[i].vpn2 & (~TLB::tlb_entry_table[i].mask)) ==
            (((_cp0_reg[CP0_ENTRYHI_REG] & 0xFFFFE000) >> 13) & (~TLB::tlb_entry_table[i].mask))) &&
            ((TLB::tlb_entry_table[i].g) ||
            (TLB::tlb_entry_table[i].asid == (_cp0_reg[CP0_ENTRYHI_REG] & 0xFF))))
        {
            _cp0_reg[CP0_INDEX_REG] = i;
            break;
        }
    }
    ++_PC;
}

void MPPInterpreter::ERET(void)
{
    _cp0->update_count(_PC);
    if (_cp0_reg[CP0_STATUS_REG] & 0x4)
    {
        LOG_ERROR("Error in ERET");
        Bus::stop = true;
    }
    else
    {
        _cp0_reg[CP0_STATUS_REG] &= ~0x2;
        global_jump_to(_cp0_reg[CP0_EPC_REG]);
    }
    _llbit = 0;
    Bus::interrupt->check_interrupt();
    Bus::last_jump_addr = _PC;
    if (Bus::next_interrupt <= _cp0_reg[CP0_COUNT_REG])
        Bus::interrupt->gen_interrupt();
}
