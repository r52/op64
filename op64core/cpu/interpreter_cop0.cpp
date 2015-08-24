#include <oputil.h>
#include <oplog.h>

#include <cpu/interpreter.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>

#include <core/bus.h>
#include <tlb/tlb.h>


void Interpreter::MFC0(void)
{
    switch (_cur_instr.rd)
    {
    case CP0_COUNT_REG:
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
    default:
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>(Bus::state.cp0_reg[_cur_instr.rd]);
        break;
    }

    ++Bus::state.PC;
}

void Interpreter::MTC0(void)
{
    switch (_cur_instr.rd)
    {
    case CP0_INDEX_REG:
        Bus::state.cp0_reg[CP0_INDEX_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x8000003F;
        if ((Bus::state.cp0_reg[CP0_INDEX_REG] & 0x3F) > 31)
        {
            LOG_ERROR(Interpreter) << "MTC0 instruction writing Index register with TLB index > 31";
            CoreControl::stop = true;
        }
        break;
    case CP0_RANDOM_REG:
        break;
    case CP0_ENTRYLO0_REG:
        Bus::state.cp0_reg[CP0_ENTRYLO0_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_ENTRYLO1_REG:
        Bus::state.cp0_reg[CP0_ENTRYLO1_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_CONTEXT_REG:
        Bus::state.cp0_reg[CP0_CONTEXT_REG] = ((uint32_t)_reg[_cur_instr.rt].u & 0xFF800000)
            | (Bus::state.cp0_reg[CP0_CONTEXT_REG] & 0x007FFFF0);
        break;
    case CP0_PAGEMASK_REG:
        Bus::state.cp0_reg[CP0_PAGEMASK_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_WIRED_REG:
        Bus::state.cp0_reg[CP0_WIRED_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        Bus::state.cp0_reg[CP0_RANDOM_REG] = 31;
        break;
    case CP0_BADVADDR_REG:
        break;
    case CP0_COUNT_REG:
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());

        Bus::state.interrupt_unsafe_state = true;
        if (Bus::state.next_interrupt <= Bus::state.cp0_reg[CP0_COUNT_REG])
            _bus->interrupt->generateInterrupt();

        Bus::state.interrupt_unsafe_state = false;

        _bus->interrupt->translateEventQueueBy((uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF);
        Bus::state.cp0_reg[CP0_COUNT_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_ENTRYHI_REG:
        Bus::state.cp0_reg[CP0_ENTRYHI_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_COMPARE_REG:
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
        _bus->interrupt->deleteEvent(COMPARE_INT);
        _bus->interrupt->addInterruptEventCount(COMPARE_INT, (uint32_t)_reg[_cur_instr.rt].u);
        Bus::state.cp0_reg[CP0_COMPARE_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        Bus::state.cp0_reg[CP0_CAUSE_REG] &= 0xFFFF7FFF; //Timer interupt is clear
        break;
    case CP0_STATUS_REG:
        if (((uint32_t)_reg[_cur_instr.rt].u & 0x04000000) != (Bus::state.cp0_reg[CP0_STATUS_REG] & 0x04000000))
        {
            _cp1.shuffleFPRData(*this, Bus::state.cp0_reg[CP0_STATUS_REG], (uint32_t)_reg[_cur_instr.rt].u);
            _cp1.setFPRPointers(*this, (uint32_t)_reg[_cur_instr.rt].u);
        }
        Bus::state.cp0_reg[CP0_STATUS_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
        ++Bus::state.PC;
        _bus->interrupt->checkInterrupt();

        Bus::state.interrupt_unsafe_state = true;
        if (Bus::state.next_interrupt <= Bus::state.cp0_reg[CP0_COUNT_REG])
            _bus->interrupt->generateInterrupt();

        Bus::state.interrupt_unsafe_state = false;
        --Bus::state.PC;
        break;
    case CP0_CAUSE_REG:
        if ((int64_t)_reg[_cur_instr.rt].s != 0)
        {
            LOG_ERROR(Interpreter) << "MTC0 instruction trying to write Cause register with non-0 value";
            CoreControl::stop = true;
        }
        else
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        }
        break;
    case CP0_EPC_REG:
        Bus::state.cp0_reg[CP0_EPC_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_PREVID_REG:
        break;
    case CP0_CONFIG_REG:
        Bus::state.cp0_reg[CP0_CONFIG_REG] = (uint32_t)_reg[_cur_instr.rt].u;
        break;
    case CP0_WATCHLO_REG:
        Bus::state.cp0_reg[CP0_WATCHLO_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_WATCHHI_REG:
        Bus::state.cp0_reg[CP0_WATCHHI_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0xFFFFFFFF;
        break;
    case CP0_TAGLO_REG:
        Bus::state.cp0_reg[CP0_TAGLO_REG] = (uint32_t)_reg[_cur_instr.rt].u & 0x0FFFFFC0;
        break;
    case CP0_TAGHI_REG:
        Bus::state.cp0_reg[CP0_TAGHI_REG] = 0;
        break;
    default:
        LOG_ERROR(Interpreter) << "Unknown MTC0 write: " << _cur_instr.fs;
        CoreControl::stop = true;
        break;
    }

    ++Bus::state.PC;
}

void Interpreter::TLBR(void)
{
    unsigned index = Bus::state.cp0_reg[CP0_INDEX_REG] & 0x3F;
    uint64_t entry_hi;

    uint32_t page_mask = (_cp0.page_mask[index] << 1) & 0x01FFE000U;
    uint32_t pfn0 = _cp0.pfn[index][0];
    uint32_t pfn1 = _cp0.pfn[index][1];
    uint8_t state0 = _cp0.state[index][0];
    uint8_t state1 = _cp0.state[index][1];

    TLB::tlb_read(_cp0.tlb, index, &entry_hi);
    Bus::state.cp0_reg[CP0_ENTRYHI_REG] = entry_hi;
    Bus::state.cp0_reg[CP0_ENTRYLO0_REG] = (pfn0 >> 6) | state0;
    Bus::state.cp0_reg[CP0_ENTRYLO1_REG] = (pfn1 >> 6) | state1;
    Bus::state.cp0_reg[CP0_PAGEMASK_REG] = page_mask;

    ++Bus::state.PC;
}

void Interpreter::TLBWI(void)
{
    uint64_t entry_hi = Bus::state.cp0_reg[CP0_ENTRYHI_REG] & 0xC00000FFFFFFE0FFULL;
    uint64_t entry_lo_0 = Bus::state.cp0_reg[CP0_ENTRYLO0_REG] & 0x000000007FFFFFFFULL;
    uint64_t entry_lo_1 = Bus::state.cp0_reg[CP0_ENTRYLO1_REG] & 0x000000007FFFFFFFULL;
    uint32_t page_mask = Bus::state.cp0_reg[CP0_PAGEMASK_REG] & 0x0000000001FFE000ULL;
    unsigned index = Bus::state.cp0_reg[CP0_INDEX_REG] & 0x3F;

    TLB::tlb_write(_cp0.tlb, index, entry_hi, entry_lo_0, entry_lo_1, page_mask);

    _cp0.page_mask[index] = (page_mask | 0x1FFF) >> 1;
    _cp0.pfn[index][0] = (entry_lo_0 << 6) & ~0xFFFU;
    _cp0.pfn[index][1] = (entry_lo_1 << 6) & ~0xFFFU;
    _cp0.state[index][0] = entry_lo_0 & 0x3F;
    _cp0.state[index][1] = entry_lo_1 & 0x3F;

    ++Bus::state.PC;
}

void Interpreter::TLBWR(void)
{
    uint64_t entry_hi = Bus::state.cp0_reg[CP0_ENTRYHI_REG] & 0xC00000FFFFFFE0FFULL;
    uint64_t entry_lo_0 = Bus::state.cp0_reg[CP0_ENTRYLO0_REG] & 0x000000007FFFFFFFULL;
    uint64_t entry_lo_1 = Bus::state.cp0_reg[CP0_ENTRYLO1_REG] & 0x000000007FFFFFFFULL;
    uint32_t page_mask = Bus::state.cp0_reg[CP0_PAGEMASK_REG] & 0x0000000001FFE000ULL;
    unsigned index = Bus::state.cp0_reg[CP0_WIRED_REG] & 0x3F;

    index = rand() % (32 - index) + index;
    TLB::tlb_write(_cp0.tlb, index, entry_hi, entry_lo_0, entry_lo_1, page_mask);

    _cp0.page_mask[index] = (page_mask | 0x1FFF) >> 1;
    _cp0.pfn[index][0] = (entry_lo_0 << 6) & ~0xFFFU;
    _cp0.pfn[index][1] = (entry_lo_1 << 6) & ~0xFFFU;
    _cp0.state[index][0] = entry_lo_0 & 0x3F;
    _cp0.state[index][1] = entry_lo_1 & 0x3F;

    ++Bus::state.PC;
}

void Interpreter::TLBP(void)
{
    uint64_t entry_hi = Bus::state.cp0_reg[CP0_ENTRYHI_REG] & 0x0000000001FFE000ULL;
    unsigned index;

    Bus::state.cp0_reg[CP0_INDEX_REG] |= 0x80000000;

    if (!TLB::tlb_probe(_cp0.tlb, entry_hi, entry_hi & 0xFF, &index))
    {
        Bus::state.cp0_reg[CP0_INDEX_REG] = index;
    }

    ++Bus::state.PC;
}

void Interpreter::ERET(void)
{
    _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
    if (Bus::state.cp0_reg[CP0_STATUS_REG] & 0x4)
    {
        Bus::state.cp0_reg[CP0_STATUS_REG] &= ~0x4;
        globalJump(Bus::state.cp0_reg[CP0_ERROREPC_REG]);
    }
    else
    {
        Bus::state.cp0_reg[CP0_STATUS_REG] &= ~0x2;
        globalJump(Bus::state.cp0_reg[CP0_EPC_REG]);
    }

    _llbit = 0;
    _bus->interrupt->checkInterrupt();
    Bus::state.last_jump_addr = Bus::state.PC;
    if (Bus::state.next_interrupt <= Bus::state.cp0_reg[CP0_COUNT_REG])
        _bus->interrupt->generateInterrupt();
}
