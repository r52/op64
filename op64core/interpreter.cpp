#include <cstring>
#include <cstdint>

#include "interpreter.h"
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

Interpreter::Interpreter(void) :
_check_nop(false)
{
}


Interpreter::~Interpreter(void)
{
    _cp0_reg = nullptr;
}


void Interpreter::initialize(void)
{
    LOG_INFO("Interpreter: initializing...");

    // Disable fpu exceptions
    _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK);

    if (_non_ieee_mode)
    {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }

    _cp0_reg = Bus::cp0_reg;
    hardReset();
    softReset();
}

void Interpreter::hardReset(void)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        _reg[i].u = 0;
        _cp0_reg[i] = 0;
        _fgr[i] = 0;
    }

    TLB::tlb_init(_cp0->tlb);

    _llbit = 0;
    _hi.u = 0;
    _lo.u = 0;
    _FCR0 = 0x511;
    _FCR31 = 0;

    _cp0_reg[CP0_RANDOM_REG] = 0x1F;
    _cp0_reg[CP0_STATUS_REG] = 0x34000000;
    _cp1->setFPRPointers(_cp0_reg[CP0_STATUS_REG]);
    _cp0_reg[CP0_CONFIG_REG] = 0x0006E463;
    _cp0_reg[CP0_PREVID_REG] = 0xb00;
    _cp0_reg[CP0_COUNT_REG] = 0x5000;
    _cp0_reg[CP0_CAUSE_REG] = 0x0000005C;
    _cp0_reg[CP0_CONTEXT_REG] = 0x007FFFF0;
    _cp0_reg[CP0_EPC_REG] = 0xFFFFFFFF;
    _cp0_reg[CP0_BADVADDR_REG] = 0xFFFFFFFF;
    _cp0_reg[CP0_ERROREPC_REG] = 0xFFFFFFFF;

    rounding_mode = ROUND_MODE;
}

static uint32_t get_system_type_reg(SystemType type)
{
    switch (type)
    {
    default:
    case SYSTEM_NTSC:
        return 1;
        break;
    case SYSTEM_PAL:
        return 0;
        break;
    case SYSTEM_MPAL:
        return 2;
        break;
    }
}

static uint32_t get_cic_seed(uint32_t chip)
{
    switch (chip)
    {
    default:
    case CIC_UNKNOWN:
    case CIC_NUS_6101:
    case CIC_NUS_6102:
        return 0x3f;
        break;
    case CIC_NUS_6103:
        return 0x78;
        break;
    case CIC_NUS_6105:
        return 0x91;
        break;
    case CIC_NUS_6106:
        return 0x85;
        break;
    }
}

void Interpreter::softReset(void)
{
    _cp0_reg[CP0_STATUS_REG] = 0x34000000;
    _cp0_reg[CP0_CONFIG_REG] = 0x0006E463;

    Bus::sp_reg[SP_STATUS_REG] = 1;
    Bus::sp_reg[SP_PC_REG] = 0;

    uint32_t bsd_dom1_config = *(uint32_t*)Bus::rom->getImage();
    Bus::pi_reg[PI_BSD_DOM1_LAT_REG] = bsd_dom1_config & 0xff;
    Bus::pi_reg[PI_BSD_DOM1_PWD_REG] = (bsd_dom1_config >> 8) & 0xff;
    Bus::pi_reg[PI_BSD_DOM1_PGS_REG] = (bsd_dom1_config >> 16) & 0x0f;
    Bus::pi_reg[PI_BSD_DOM1_RLS_REG] = (bsd_dom1_config >> 20) & 0x03;
    Bus::pi_reg[PI_STATUS_REG] = 0;

    Bus::ai_reg[AI_DRAM_ADDR_REG] = 0;
    Bus::ai_reg[AI_LEN_REG] = 0;

    Bus::vi_reg[VI_INTR_REG] = 1023;
    Bus::vi_reg[VI_CURRENT_REG] = 0;
    Bus::vi_reg[VI_H_START_REG] = 0;

    Bus::mi_reg[MI_INTR_REG] &= ~(0x10 | 0x8 | 0x4 | 0x1);

    // copy boot code
    memcpy(Bus::sp_dmem8 + 0x40, Bus::rom->getImage() + 0x40, 0xFC0);

    _reg[19].u = 0; // 0:Cart, 1:DD
    _reg[20].u = get_system_type_reg(Bus::rom->getSystemType());    // 0:PAL, 1:NTSC, 2:MPAL
    _reg[21].u = 0; // 0:ColdReset, 1:NMI
    _reg[22].u = get_cic_seed(Bus::rom->getCICChip());
    _reg[23].u = 0;

    uint32_t* sp_imem = Bus::sp_imem32;

    // required by CIC x105
    sp_imem[0] = 0x3C0DBFC0;
    sp_imem[1] = 0x8DA807FC;
    sp_imem[2] = 0x25AD07C0;
    sp_imem[3] = 0x31080080;
    sp_imem[4] = 0x5500FFFC;
    sp_imem[5] = 0x3C0DBFC0;
    sp_imem[6] = 0x8DA80024;
    sp_imem[7] = 0x3C0BB000;

    // required by CIC x105
    _reg[11].u = 0xFFFFFFFFA4000040;
    _reg[29].u = 0xFFFFFFFFA4001FF0;
    _reg[31].u = 0xFFFFFFFFA4001550;
}

void Interpreter::execute(void)
{
    LOG_INFO("Interpreter: running...");
    _delay_slot = false;
    Bus::stop = false;
    Bus::skip_jump = 0;

    _PC = Bus::last_jump_addr = 0xa4000040;
    Bus::next_interrupt = 624999;
    Bus::interrupt->initialize();

    while (!Bus::stop)
    {
        prefetch();

        (this->*instruction_table[_cur_instr.op])();
    }
}

void Interpreter::J(void)
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

void Interpreter::JAL(void)
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

void Interpreter::BEQ(void)
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

void Interpreter::BNE(void)
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

void Interpreter::BLEZ(void)
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

void Interpreter::BGTZ(void)
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

void Interpreter::ADDI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + signextend<int16_t, int32_t>(_cur_instr.immediate));
    }

    ++_PC;
}

void Interpreter::ADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + signextend<int16_t, int32_t>(_cur_instr.immediate));
    }

    ++_PC;
}

void Interpreter::SLTI(void)
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

void Interpreter::SLTIU(void)
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

void Interpreter::ANDI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s & _cur_instr.immediate;
    
    ++_PC;
}

void Interpreter::ORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s | _cur_instr.immediate;
    
    ++_PC;
}

void Interpreter::XORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s ^ _cur_instr.immediate;
    
    ++_PC;
}

void Interpreter::LUI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int16_t)_cur_instr.immediate << 16);
    }
    
    ++_PC;
}

void Interpreter::SV(void)
{
    Bus::stop = true;
    LOG_ERROR("OP: %x; Opcode %u reserved. Stopping...", _cur_instr.code, _cur_instr.op);
}

void Interpreter::BEQL(void)
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

void Interpreter::BNEL(void)
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

void Interpreter::BLEZL(void)
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

void Interpreter::BGTZL(void)
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

void Interpreter::DADDI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s + signextend<int16_t, int64_t>(_cur_instr.immediate);
    }
    ++_PC;
}

void Interpreter::DADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s + signextend<int16_t, int64_t>(_cur_instr.immediate);
    }

    ++_PC;
}

void Interpreter::LDL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~7;
    uint64_t value = 0;
    uint32_t offset = addr & 7;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_DWORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (_reg[_cur_instr.rt].s & LDL_MASK[offset]);
        _reg[_cur_instr.rt].s += (value << LDL_SHIFT[offset]);
    }
}

void Interpreter::LDR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~7;
    uint64_t value = 0;
    uint32_t offset = addr & 7;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_DWORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (_reg[_cur_instr.rt].s & LDR_MASK[offset]);
        _reg[_cur_instr.rt].s += (value >> LDR_SHIFT[offset]);
    }
}

void Interpreter::LB(void)
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

void Interpreter::LH(void)
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

void Interpreter::LWL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (int32_t)(_reg[_cur_instr.rt].s & LWL_MASK[offset]);
        _reg[_cur_instr.rt].s += (int32_t)(value << LWL_SHIFT[offset]);
    }
}

void Interpreter::LW(void)
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

void Interpreter::LBU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
	
        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_BYTE);
    }
    ++_PC;
}

void Interpreter::LHU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_HWORD);
    }
    ++_PC;
}

void Interpreter::LWR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;
    ++_PC;

    Bus::mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (int32_t)(_reg[_cur_instr.rt].s & LWR_MASK[offset]);
        _reg[_cur_instr.rt].s += (int32_t)(value >> LWR_SHIFT[offset]);
    }

}

void Interpreter::LWU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_WORD);
    }
    ++_PC;
}

void Interpreter::SB(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint8_t)(_reg[_cur_instr.rt].u & 0xFF), SIZE_BYTE);
}

void Interpreter::SH(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint16_t)(_reg[_cur_instr.rt].u & 0xFFFF), SIZE_HWORD);
}

void Interpreter::SWL(void)
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
        value += (uint32_t)(_reg[_cur_instr.rt].u >> SWL_SHIFT[offset]);

        Bus::mem->writemem(addr & ~0x03, value, SIZE_WORD);
    }
}

void Interpreter::SW(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint32_t)(_reg[_cur_instr.rt].u & 0xFFFFFFFF), SIZE_WORD);
}

void Interpreter::SDL(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SDR(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SWR(void)
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
        value += (uint32_t)(_reg[_cur_instr.rt].u << SWR_SHIFT[offset]);

        Bus::mem->writemem(addr & ~0x03, value, SIZE_WORD);
    }
}

void Interpreter::CACHE(void)
{
    ++_PC;
}

void Interpreter::LL(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::LWC1(void)
{
    if (_cp0->COP1Unusable())
        return;

    ++_PC;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint64_t dest = 0;

    Bus::mem->readmem(address, &dest, SIZE_WORD);

    if (address)
    {
        *((int32_t*)_s_reg[_cur_instr.ft]) = (int32_t)dest;
    }
}

void Interpreter::LLD(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::LDC1(void)
{
    if (_cp0->COP1Unusable())
        return;

    ++_PC;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

    Bus::mem->readmem(address, (uint64_t*)_d_reg[_cur_instr.ft], SIZE_DWORD);
}

void Interpreter::LD(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        Bus::mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_DWORD);
    }
    ++_PC;
}

void Interpreter::SC(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SWC1(void)
{
    if (_cp0->COP1Unusable())
        return;

    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int32_t*)_s_reg[_cur_instr.ft]), SIZE_WORD);
}

void Interpreter::SCD(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SDC1(void)
{
    if (_cp0->COP1Unusable())
        return;

    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int64_t*)_d_reg[_cur_instr.ft]), SIZE_DWORD);
}

void Interpreter::SD(void)
{
    ++_PC;

    Bus::mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), _reg[_cur_instr.rt].u, SIZE_DWORD);
}

void Interpreter::genericIdle(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1)
{
    int32_t skip;
    if (cop1 && _cp0->COP1Unusable())
        return;

    if (take_jump)
    {
        _cp0->updateCount(_PC);
        skip = Bus::next_interrupt - _cp0_reg[CP0_COUNT_REG];
        if (skip > 3)
        {
            _cp0_reg[CP0_COUNT_REG] += (skip & 0xFFFFFFFC);
        }
        else
        {
            genericJump(destination, take_jump, link, likely, cop1);
        }
    }
    else
    {
        genericJump(destination, take_jump, link, likely, cop1);
    }
}

void Interpreter::genericJump(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1)
{
    if (cop1 && _cp0->COP1Unusable())
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

        _cp0->updateCount(_PC);
        _delay_slot = false;
        if (take_jump && !Bus::skip_jump)
        {
            _PC = destination;
        }
    }
    else
    {
        _PC += 2;
        _cp0->updateCount(_PC);
    }

    Bus::last_jump_addr = (uint32_t)_PC;
    if (Bus::next_interrupt <= _cp0_reg[CP0_COUNT_REG])
    {
        Bus::interrupt->generateInterrupt();
    }
}

void Interpreter::globalJump(uint32_t addr)
{
    _PC = addr;
}

void Interpreter::generalException(void)
{
    _cp0->updateCount(_PC);
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

    globalJump(0x80000180);
    Bus::last_jump_addr = (uint32_t)_PC;

    if (_delay_slot)
    {
        Bus::skip_jump = (uint32_t)_PC;
        Bus::next_interrupt = 0;
    }

}

void Interpreter::TLBRefillException(unsigned int address, TLBProbeMode mode)
{
    bool usual_handler = false;

    if (mode != TLB_FAST_READ)
    {
        _cp0->updateCount(_PC);
    }

    if (mode == TLB_WRITE)
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
        globalJump(0x80000180);
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
            usual_handler = true;
        }

        if (usual_handler)
        {
            globalJump(0x80000180);
        }
        else
        {
            globalJump(0x80000000);
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

    if (mode != TLB_FAST_READ)
    {
        _cp0_reg[CP0_EPC_REG] -= 4;
    }

    Bus::last_jump_addr = (uint32_t)_PC;

    if (_delay_slot)
    {
        Bus::skip_jump = (uint32_t)_PC;
        Bus::next_interrupt = 0;
    }
}
