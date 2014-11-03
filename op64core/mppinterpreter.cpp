#include <cstring>
#include <cstdint>

#include "mppinterpreter.h"
#include "cp0.h"
#include "cp1.h"
#include "rom.h"
#include "interrupthandler.h"
#include "util.h"
#include "tlb.h"

// borrowed from pj64
static const uint32_t SWL_MASK[4] = { 0x00000000, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
static const uint32_t SWR_MASK[4] = { 0x00FFFFFF, 0x0000FFFF, 0x000000FF, 0x00000000 };
static const uint32_t LWL_MASK[4] = { 0x00000000, 0x000000FF, 0x0000FFFF, 0x00FFFFFF };
static const uint32_t LWR_MASK[4] = { 0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0x0000000 };
static const uint64_t SDL_MASK[8] = { 
    0, 
    0xFF00000000000000, 
    0xFFFF000000000000, 
    0xFFFFFF0000000000,
    0xFFFFFFFF00000000,
    0xFFFFFFFFFF000000,
    0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFFFFFF00
};
static const uint64_t SDR_MASK[8] = { 
    0x00FFFFFFFFFFFFFF,
    0x0000FFFFFFFFFFFF,
    0x000000FFFFFFFFFF,
    0x00000000FFFFFFFF,
    0x0000000000FFFFFF,
    0x000000000000FFFF,
    0x00000000000000FF,
    0x0000000000000000
};
static const uint64_t LDL_MASK[8] = { 
    0,
    0xFF,
    0xFFFF,
    0xFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFFFF,
    0xFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFF
};
static const uint64_t LDR_MASK[8] = {
    0xFFFFFFFFFFFFFF00,
    0xFFFFFFFFFFFF0000,
    0xFFFFFFFFFF000000,
    0xFFFFFFFF00000000,
    0xFFFFFF0000000000,
    0xFFFF000000000000,
    0xFF00000000000000,
    0
};

const int SWL_SHIFT[4] = { 0, 8, 16, 24 };
const int SWR_SHIFT[4] = { 24, 16, 8, 0 };
const int LWL_SHIFT[4] = { 0, 8, 16, 24 };
const int LWR_SHIFT[4] = { 24, 16, 8, 0 };
const int SDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
const int SDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };
const int LDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
const int LDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

MPPInterpreter::MPPInterpreter(void) :
_check_nop(false),
_delay_slot(false),
_skip_jump(0)
{
    // forward internals
    using namespace Bus;
    last_instr_addr = &_last_instr_addr;
    delay_slot = &_delay_slot;
    skip_jump = &_skip_jump;
}


MPPInterpreter::~MPPInterpreter(void)
{
    _cp0_reg = nullptr;

    using namespace Bus;
    last_instr_addr = nullptr;
    delay_slot = nullptr;
    skip_jump = nullptr;
}


void MPPInterpreter::initialize(void)
{
    LOG_INFO("Interpreter: initializing...");
    _cp0_reg = Bus::cp0_reg;
    _fgr = Bus::fgr;
    hard_reset();
    soft_reset();
}

void MPPInterpreter::hard_reset(void)
{
    vec_for (uint32_t i = 0; i < 32; i++)
    {
        _reg[i].u = 0;
        _cp0_reg[i] = 0;
        _fgr[i] = 0;

        TLB::tlb_entry_table[i].mask = 0;
        TLB::tlb_entry_table[i].vpn2 = 0;
        TLB::tlb_entry_table[i].g = 0;
        TLB::tlb_entry_table[i].asid = 0;
        TLB::tlb_entry_table[i].pfn_even = 0;
        TLB::tlb_entry_table[i].c_even = 0;
        TLB::tlb_entry_table[i].d_even = 0;
        TLB::tlb_entry_table[i].v_even = 0;
        TLB::tlb_entry_table[i].pfn_odd = 0;
        TLB::tlb_entry_table[i].c_odd = 0;
        TLB::tlb_entry_table[i].d_odd = 0;
        TLB::tlb_entry_table[i].v_odd = 0;
        TLB::tlb_entry_table[i].r = 0;
        //TLB::tlb_entry_table[i].check_par:ty_mask=0x1000;

        TLB::tlb_entry_table[i].start_even = 0;
        TLB::tlb_entry_table[i].end_even = 0;
        TLB::tlb_entry_table[i].phys_even = 0;
        TLB::tlb_entry_table[i].start_odd = 0;
        TLB::tlb_entry_table[i].end_odd = 0;
        TLB::tlb_entry_table[i].phys_odd = 0;

        TLB::tlb_lookup_read[i] = 0;
        TLB::tlb_lookup_write[i] = 0;
    }

    _llbit = 0;
    _hi.u = 0;
    _lo.u = 0;
    *(Bus::FCR0) = 0x511;
    *(Bus::FCR31) = 0;

    _cp0_reg[CP0_RANDOM_REG] = 0x1F;
    _cp0_reg[CP0_STATUS_REG] = 0x34000000;
    _cp1->set_fpr_pointers(_cp0_reg[CP0_STATUS_REG]);
    _cp0_reg[CP0_CONFIG_REG] = 0x0006E463;
    _cp0_reg[CP0_PREVID_REG] = 0xb00;
    _cp0_reg[CP0_COUNT_REG] = 0x5000;
    _cp0_reg[CP0_CAUSE_REG] = 0x0000005C;
    _cp0_reg[CP0_CONTEXT_REG] = 0x007FFFF0;
    _cp0_reg[CP0_EPC_REG] = 0xFFFFFFFF;
    _cp0_reg[CP0_BADVADDR_REG] = 0xFFFFFFFF;
    _cp0_reg[CP0_ERROREPC_REG] = 0xFFFFFFFF;

    _cp1->rounding_mode = ROUND_MODE;

    // Disable fpu exceptions
    _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK);

    if (_non_ieee_mode)
    {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }
}

void MPPInterpreter::soft_reset(void)
{
    // copy boot code
    memcpy(Bus::sp_dmem8 + 0x40, Bus::rom->getImage() + 0x40, 0xFBC);

    _reg[6].s = 0xFFFFFFFFA4001F0C;
    _reg[7].s = 0xFFFFFFFFA4001F08;
    _reg[8].s = 0x00000000000000C0;
    _reg[10].s = 0x0000000000000040;
    _reg[11].s = 0xFFFFFFFFA4000040;
    _reg[29].s = 0xFFFFFFFFA4001FF0;


    uint32_t* sp_dmem = Bus::sp_dmem32;
    uint32_t* sp_imem = Bus::sp_imem32;    

    switch (Bus::rom->getSystemType())
    {
    case SYSTEM_PAL:
    {
        switch (Bus::rom->getCICChip()) {
        case CIC_NUS_6102:
            _reg[5].s = 0xFFFFFFFFC0F1D859;
            _reg[14].s = 0x000000002DE108EA;
            break;
        case CIC_NUS_6103:
            _reg[5].s = 0xFFFFFFFFD4646273;
            _reg[14].s = 0x000000001AF99984;
            break;
        case CIC_NUS_6105:
            sp_imem[1] = 0xBDA807FC;
            _reg[5].s = 0xFFFFFFFFDECAAAD1;
            _reg[14].s = 0x000000000CF85C13;
            _reg[24].s = 0x0000000000000002;
            break;
        case CIC_NUS_6106:
            _reg[5].s = 0xFFFFFFFFB04DC903;
            _reg[14].s = 0x000000001AF99984;
            _reg[24].s = 0x0000000000000002;
            break;
        }
        _reg[23].s = 0x0000000000000006;
        _reg[31].s = 0xFFFFFFFFA4001554;
    }
        break;
    case SYSTEM_NTSC:
    default:
    {
        switch (Bus::rom->getCICChip()) {
        case CIC_NUS_6102:
            _reg[5].s = 0xFFFFFFFFC95973D5;
            _reg[14].s = 0x000000002449A366;
            break;
        case CIC_NUS_6103:
            _reg[5].s = 0xFFFFFFFF95315A28;
            _reg[14].s = 0x000000005BACA1DF;
            break;
        case CIC_NUS_6105:
            sp_imem[1] = 0x8DA807FC;
            _reg[5].s = 0x000000005493FB9A;
            _reg[14].s = 0xFFFFFFFFC2C20384;
            break;
        case CIC_NUS_6106:
            _reg[5].s = 0xFFFFFFFFE067221F;
            _reg[14].s = 0x000000005CD2B70F;
            break;
        }
        _reg[20].s = 0x0000000000000001;
        _reg[24].s = 0x0000000000000003;
        _reg[31].s = 0xFFFFFFFFA4001550;
    }
        break;
    }


    switch (Bus::rom->getCICChip()) {
    case CIC_NUS_6101:
        _reg[22].s = 0x000000000000003F;
        break;
    case CIC_NUS_6102:
        _reg[1].s = 0x0000000000000001;
        _reg[2].s = 0x000000000EBDA536;
        _reg[3].s = 0x000000000EBDA536;
        _reg[4].s = 0x000000000000A536;
        _reg[12].s = 0xFFFFFFFFED10D0B3;
        _reg[13].s = 0x000000001402A4CC;
        _reg[15].s = 0x000000003103E121;
        _reg[22].s = 0x000000000000003F;
        _reg[25].s = 0xFFFFFFFF9DEBB54F;
        break;
    case CIC_NUS_6103:
        _reg[1].s = 0x0000000000000001;
        _reg[2].s = 0x0000000049A5EE96;
        _reg[3].s = 0x0000000049A5EE96;
        _reg[4].s = 0x000000000000EE96;
        _reg[12].s = 0xFFFFFFFFCE9DFBF7;
        _reg[13].s = 0xFFFFFFFFCE9DFBF7;
        _reg[15].s = 0x0000000018B63D28;
        _reg[22].s = 0x0000000000000078;
        _reg[25].s = 0xFFFFFFFF825B21C9;
        break;
    case CIC_NUS_6105:
        sp_imem[0] = 0x3C0DBFC0;
        sp_imem[2] = 0x25AD07C0;
        sp_imem[3] = 0x31080080;
        sp_imem[4] = 0x5500FFFC;
        sp_imem[5] = 0x3C0DBFC0;
        sp_imem[6] = 0x8DA80024;
        sp_imem[7] = 0x3C0BB000;
        _reg[2].s = 0xFFFFFFFFF58B0FBF;
        _reg[3].s = 0xFFFFFFFFF58B0FBF;
        _reg[4].s = 0x0000000000000FBF;
        _reg[12].s = 0xFFFFFFFF9651F81E;
        _reg[13].s = 0x000000002D42AAC5;
        _reg[15].s = 0x0000000056584D60;
        _reg[22].s = 0x0000000000000091;
        _reg[25].s = 0xFFFFFFFFCDCE565F;
        break;
    case CIC_NUS_6106:
        _reg[2].s = 0xFFFFFFFFA95930A4;
        _reg[3].s = 0xFFFFFFFFA95930A4;
        _reg[4].s = 0x00000000000030A4;
        _reg[12].s = 0xFFFFFFFFBCB59510;
        _reg[13].s = 0xFFFFFFFFBCB59510;
        _reg[15].s = 0x000000007A3C07F4;
        _reg[22].s = 0x0000000000000085;
        _reg[25].s = 0x00000000465E3F72;
        break;
    }
}

void MPPInterpreter::execute(void)
{
    LOG_INFO("Interpreter: running...");
    _delay_slot = false;
    Bus::stop = false;

    _PC = _last_instr_addr = 0xa4000040;
    *(Bus::next_interrupt) = 624999;
    Bus::interrupt->initialize();

    while (!Bus::stop)
    {
        prefetch();

        (this->*instruction_table[_cur_instr.op])();
    }
}

void MPPInterpreter::SPECIAL(void)
{
    (this->*special_table[_cur_instr.func])();
}

void MPPInterpreter::REGIMM(void)
{
    (this->*regimm_table[_cur_instr.rt])();
}

void MPPInterpreter::J(void)
{
    // DECLARE_JUMP(J,   (PC->f.j.inst_index<<2) | ((PCADDR+4) & 0xF0000000), 1, &reg[0],  0, 0)
    DO_JUMP(
        ((_cur_instr.target << 2) | (((uint32_t)_PC + 4) & 0xF0000000)),
        true,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::JAL(void)
{
    // DECLARE_JUMP(JAL, (PC->f.j.inst_index<<2) | ((PCADDR+4) & 0xF0000000), 1, &reg[31], 0, 0)
    DO_JUMP(
        ((_cur_instr.target << 2) | (((uint32_t)_PC + 4) & 0xF0000000)),
        true,
        &_reg[31],
        false,
        false
        );
}

void MPPInterpreter::BEQ(void)
{
    //DECLARE_JUMP(BEQ, PCADDR + (iimmediate + 1) * 4, irs == irt, &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s == _reg[_cur_instr.rt].s,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::BNE(void)
{
    // DECLARE_JUMP(BNE, PCADDR + (iimmediate+1)*4, irs != irt, &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s != _reg[_cur_instr.rt].s,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::BLEZ(void)
{
    //DECLARE_JUMP(BLEZ,    PCADDR + (iimmediate+1)*4, irs <= 0,   &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s <= 0,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::BGTZ(void)
{
    // DECLARE_JUMP(BGTZ, PCADDR + (iimmediate + 1) * 4, irs > 0, &reg[0], 0, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s > 0,
        &_reg[0],
        false,
        false
        );
}

void MPPInterpreter::ADDI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + signextend<int16_t, int32_t>(_cur_instr.immediate));
    }

    ++_PC;
}

void MPPInterpreter::ADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + signextend<int16_t, int32_t>(_cur_instr.immediate));
    }

    ++_PC;
}

void MPPInterpreter::SLTI(void)
{
    if (_reg[_cur_instr.rs].s < signextend<int16_t, int64_t>(_cur_instr.immediate))
    {
        _reg[_cur_instr.rt].s = 1;
    }
    else
    {
        _reg[_cur_instr.rt].s = 0;
    }
    
    ++_PC;
}

void MPPInterpreter::SLTIU(void)
{
    if (_reg[_cur_instr.rs].u < ((uint64_t)signextend<int16_t, int64_t>(_cur_instr.immediate)))
    {
        _reg[_cur_instr.rt].s = 1;
    }
    else
    {
        _reg[_cur_instr.rt].s = 0;
    }

    ++_PC;
}

void MPPInterpreter::ANDI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s & _cur_instr.immediate;
    
    ++_PC;
}

void MPPInterpreter::ORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s | _cur_instr.immediate;
    
    ++_PC;
}

void MPPInterpreter::XORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s ^ _cur_instr.immediate;
    
    ++_PC;
}

void MPPInterpreter::LUI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int16_t)_cur_instr.immediate << 16);
    }
    
    ++_PC;
}

void MPPInterpreter::COP0(void)
{
    (this->*cop0_table[_cur_instr.rs])();
}

void MPPInterpreter::COP1(void)
{
    (this->*cop1_table[_cur_instr.fmt])();
}

void MPPInterpreter::SV(void)
{
    Bus::stop = true;
    LOG_ERROR("OP: %x; Opcode %u reserved. Stopping...", _cur_instr.code, _cur_instr.op);
}

void MPPInterpreter::BEQL(void)
{
    //DECLARE_JUMP(BEQL, PCADDR + (iimmediate + 1) * 4, irs == irt, &reg[0], 1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s == _reg[_cur_instr.rt].s,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::BNEL(void)
{
    //DECLARE_JUMP(BNEL, PCADDR + (iimmediate + 1) * 4, irs != irt, &reg[0], 1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s != _reg[_cur_instr.rt].s,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::BLEZL(void)
{
    //DECLARE_JUMP(BLEZL,   PCADDR + (iimmediate+1)*4, irs <= 0,   &reg[0], 1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s <= 0,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::BGTZL(void)
{
    // DECLARE_JUMP(BGTZL,   PCADDR + (iimmediate+1)*4, irs > 0,    &reg[0], 1, 0)
    DO_JUMP(
        ((uint32_t)_PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
        _reg[_cur_instr.rs].s > 0,
        &_reg[0],
        true,
        false
        );
}

void MPPInterpreter::DADDI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s + signextend<int16_t, int64_t>(_cur_instr.immediate);
    }
    ++_PC;
}

void MPPInterpreter::DADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s + signextend<int16_t, int64_t>(_cur_instr.immediate);
    }

    ++_PC;
}

void MPPInterpreter::LDL(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::LDR(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::LB(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_BYTE);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int8_t, int64_t>((int8_t)_reg[_cur_instr.rt].s);
        }
    }
    ++_PC;
}

void MPPInterpreter::LH(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_HWORD);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int16_t, int64_t>((int16_t)_reg[_cur_instr.rt].s);
        }
    }
    ++_PC;
}

void MPPInterpreter::LWL(void)
{
    uint32_t addr = (uint32_t)((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = ((int32_t)_reg[_cur_instr.rt].s & LWL_MASK[offset]);
        _reg[_cur_instr.rt].s += (signextend<int32_t, int64_t>((int32_t)value) << LWL_SHIFT[offset]);
    }
}

void MPPInterpreter::LW(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_WORD);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s);
        }
    }
    ++_PC;
}

void MPPInterpreter::LBU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
	
        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_BYTE);
    }
    ++_PC;
}

void MPPInterpreter::LHU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_HWORD);
    }
    ++_PC;
}

void MPPInterpreter::LWR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = ((int32_t)_reg[_cur_instr.rt].s & LWR_MASK[offset]);
        _reg[_cur_instr.rt].s += ((int32_t)value >> LWR_SHIFT[offset]);
    }

}

void MPPInterpreter::LWU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_WORD);
    }
    ++_PC;
}

void MPPInterpreter::SB(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint8_t)(_reg[_cur_instr.rt].u & 0xFF), SIZE_BYTE);
}

void MPPInterpreter::SH(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint16_t)(_reg[_cur_instr.rt].u & 0xFFFF), SIZE_HWORD);
}

void MPPInterpreter::SWL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        value &= SWL_MASK[offset];
        value += ((uint32_t)_reg[_cur_instr.rt].u >> SWL_SHIFT[offset]);

        Bus::mem->writemem(masked_addr, value, SIZE_WORD);
    }
}

void MPPInterpreter::SW(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint32_t)(_reg[_cur_instr.rt].u & 0xFFFFFFFF), SIZE_WORD);
}

void MPPInterpreter::SDL(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::SDR(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::SWR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        value &= SWR_MASK[offset];
        value += (_reg[_cur_instr.rt].u << SWR_SHIFT[offset]);

        Bus::mem->writemem(masked_addr, value, SIZE_WORD);
    }
}

void MPPInterpreter::CACHE(void)
{
    ++_PC;
}

void MPPInterpreter::LL(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::LWC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint64_t dest = 0;

    Bus::mem->readmem(address, &dest, SIZE_WORD);

    if (address)
    {
        *((int32_t*)Bus::s_reg[_cur_instr.ft]) = (int32_t)dest;
    }
}

void MPPInterpreter::LLD(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::LDC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

    Bus::mem->readmem(address, (uint64_t*)Bus::d_reg[_cur_instr.ft], SIZE_DWORD);
}

void MPPInterpreter::LD(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_DWORD);
    }
    ++_PC;
}

void MPPInterpreter::SC(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::SWC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int32_t*)Bus::s_reg[_cur_instr.ft]), SIZE_WORD);
}

void MPPInterpreter::SCD(void)
{
    NOT_IMPLEMENTED();
}

void MPPInterpreter::SDC1(void)
{
    if (_cp0->cop1_unusable())
        return;

    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int64_t*)Bus::d_reg[_cur_instr.ft]), SIZE_DWORD);
}

void MPPInterpreter::SD(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), _reg[_cur_instr.rt].u, SIZE_DWORD);
}

void MPPInterpreter::generic_idle(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1)
{
    int32_t skip;
    if (cop1 && _cp0->cop1_unusable())
        return;

    if (take_jump)
    {
        _cp0->update_count();
        skip = *(Bus::next_interrupt) - _cp0_reg[CP0_COUNT_REG];
        if (skip > 3)
        {
            _cp0_reg[CP0_COUNT_REG] += (skip & 0xFFFFFFFC);
        }
        else
        {
            generic_jump(destination, take_jump, link, likely, cop1);
        }
    }
    else
    {
        generic_jump(destination, take_jump, link, likely, cop1);
    }
}

void MPPInterpreter::generic_jump(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1)
{
    if (cop1 && _cp0->cop1_unusable())
        return;

    if (link != &_reg[0])
    {
        (*link).s = ((uint32_t)_PC) + 8;
        (*link).s = signextend<int32_t, int64_t>((int32_t)(*link).s);
    }

    if (!likely || take_jump)
    {
        _PC += 1;
        _delay_slot = true;

        prefetch();
        (this->*instruction_table[_cur_instr.op])();

        _cp0->update_count();
        _delay_slot = false;
        if (take_jump && !_skip_jump)
        {
            _PC = destination;
        }
    }
    else
    {
        _PC += 2;
        _cp0->update_count();
    }

    _last_instr_addr = (uint32_t)_PC;
    if (*(Bus::next_interrupt) <= _cp0_reg[CP0_COUNT_REG])
    {
        Bus::interrupt->gen_interrupt();
    }
}

void MPPInterpreter::global_jump_to(uint32_t addr)
{
    _PC = addr;
}

void MPPInterpreter::general_exception(void)
{
    _cp0->update_count();
    _cp0_reg[CP0_STATUS_REG] |= 2;

    _cp0_reg[CP0_EPC_REG] = (uint32_t)_PC;

    if (_delay_slot)
    {
        _cp0_reg[CP0_CAUSE_REG] |= 0x80000000;
        _cp0_reg[CP0_EPC_REG] -= 4;
    }
    else
    {
        _cp0_reg[CP0_CAUSE_REG] &= 0x7FFFFFFF;
    }

    global_jump_to(0x80000180);
    _last_instr_addr = (uint32_t)_PC;

    if (_delay_slot)
    {
        _skip_jump = (uint32_t)_PC;
        *(Bus::next_interrupt) = 0;
    }

}

void MPPInterpreter::TLB_refill_exception(unsigned int address, int w)
{
    uint32_t i;
    bool usual_handler = false;

    if (w != 2)
    {
        _cp0->update_count();
    }

    if (w == 1)
    {
        _cp0_reg[CP0_CAUSE_REG] = (3 << 2);
    }
    else
    {
        _cp0_reg[CP0_CAUSE_REG] = (2 << 2);
    }

    _cp0_reg[CP0_BADVADDR_REG] = address;
    _cp0_reg[CP0_CONTEXT_REG] = (_cp0_reg[CP0_CONTEXT_REG] & 0xFF80000F) | ((address >> 9) & 0x007FFFF0);
    _cp0_reg[CP0_ENTRYHI_REG] = address & 0xFFFFE000;

    if (_cp0_reg[CP0_STATUS_REG] & 0x2) // Test de EXL
    {
        global_jump_to(0x80000180);
        if (_delay_slot)
        {
            _cp0_reg[CP0_CAUSE_REG] |= 0x80000000;
        }
        else
        {
            _cp0_reg[CP0_CAUSE_REG] &= 0x7FFFFFFF;
        }
    }
    else
    {
        _cp0_reg[CP0_EPC_REG] = (uint32_t)_PC;

        _cp0_reg[CP0_CAUSE_REG] &= ~0x80000000;
        _cp0_reg[CP0_STATUS_REG] |= 0x2; //EXL=1

        if (address >= 0x80000000 && address < 0xc0000000)
        {
            usual_handler = 1;
        }

        for (i = 0; i < 32; i++)
        {
            if (/*tlb_e[i].v_even &&*/ address >= TLB::tlb_entry_table[i].start_even &&
                address <= TLB::tlb_entry_table[i].end_even)
            {
                usual_handler = true;
            }

            if (/*tlb_e[i].v_odd &&*/ address >= TLB::tlb_entry_table[i].start_odd &&
                address <= TLB::tlb_entry_table[i].end_odd)
            {
                usual_handler = true;
            }
        }

        if (usual_handler)
        {
            global_jump_to(0x80000180);
        }
        else
        {
            global_jump_to(0x80000000);
        }
    }

    if (_delay_slot)
    {
        _cp0_reg[CP0_CAUSE_REG] |= 0x80000000;
        _cp0_reg[CP0_EPC_REG] -= 4;
    }
    else
    {
        _cp0_reg[CP0_CAUSE_REG] &= 0x7FFFFFFF;
    }

    if (w != 2)
    {
        _cp0_reg[CP0_EPC_REG] -= 4;
    }

    _last_instr_addr = (uint32_t)_PC;

    if (_delay_slot)
    {
        *(Bus::skip_jump) = (uint32_t)_PC;
        *(Bus::next_interrupt) = 0;
    }
}
