#include <cstring>
#include <cstdint>

#include <oputil.h>

#include <cpu/interpreter.h>
#include <cpu/cp0.h>
#include <cpu/interrupthandler.h>

#include <rom/rom.h>
#include <tlb/tlb.h>


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



bool Interpreter::initialize(Bus* bus)
{
    LOG_INFO(Interpreter) << "Initializing...";

    if (nullptr == bus)
    {
        LOG_ERROR(Interpreter) << "Invalid Bus";
        return false;
    }

    // Disable fpu exceptions
    _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK);

    if (_non_ieee_mode)
    {
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }

    _bus = bus;
    bus->interrupt.reset(new InterruptHandler);

    hardReset();
    softReset();

    return true;
}

void Interpreter::uninitialize(Bus* bus)
{
    bus->interrupt.reset();
    _bus = nullptr;
}

void Interpreter::hardReset(void)
{
    for (uint32_t i = 0; i < 32; i++)
    {
        _reg[i].u = 0;
        Bus::state.cp0_reg[i] = 0;
        _fgr[i] = 0;
    }

    TLB::tlb_init(_cp0.tlb);

    _llbit = 0;
    _hi.u = 0;
    _lo.u = 0;
    _FCR0 = 0x511;
    _FCR31 = 0;

    Bus::state.cp0_reg[CP0_RANDOM_REG] = 0x1F;
    Bus::state.cp0_reg[CP0_STATUS_REG] = 0x34000000;
    _cp1.setFPRPointers(*this, Bus::state.cp0_reg[CP0_STATUS_REG]);
    Bus::state.cp0_reg[CP0_CONFIG_REG] = 0x0006E463;
    Bus::state.cp0_reg[CP0_PREVID_REG] = 0xb00;
    Bus::state.cp0_reg[CP0_COUNT_REG] = 0x5000;
    Bus::state.cp0_reg[CP0_CAUSE_REG] = 0x0000005C;
    Bus::state.cp0_reg[CP0_CONTEXT_REG] = 0x007FFFF0;
    Bus::state.cp0_reg[CP0_EPC_REG] = 0xFFFFFFFF;
    Bus::state.cp0_reg[CP0_BADVADDR_REG] = 0xFFFFFFFF;
    Bus::state.cp0_reg[CP0_ERROREPC_REG] = 0xFFFFFFFF;

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
    Bus::state.cp0_reg[CP0_STATUS_REG] = 0x34000000;
    Bus::state.cp0_reg[CP0_CONFIG_REG] = 0x0006E463;

    Bus::rcp.sp.reg[SP_STATUS_REG] = 1;
    Bus::rcp.sp.stat[SP_PC_REG] = 0;

    uint32_t bsd_dom1_config = *(uint32_t*)_bus->rom->getImage();
    Bus::rcp.pi.reg[PI_BSD_DOM1_LAT_REG] = bsd_dom1_config & 0xff;
    Bus::rcp.pi.reg[PI_BSD_DOM1_PWD_REG] = (bsd_dom1_config >> 8) & 0xff;
    Bus::rcp.pi.reg[PI_BSD_DOM1_PGS_REG] = (bsd_dom1_config >> 16) & 0x0f;
    Bus::rcp.pi.reg[PI_BSD_DOM1_RLS_REG] = (bsd_dom1_config >> 20) & 0x03;
    Bus::rcp.pi.reg[PI_STATUS_REG] = 0;

    Bus::rcp.ai.reg[AI_DRAM_ADDR_REG] = 0;
    Bus::rcp.ai.reg[AI_LEN_REG] = 0;

    Bus::rcp.vi.reg[VI_INTR_REG] = 1023;
    Bus::rcp.vi.reg[VI_CURRENT_REG] = 0;
    Bus::rcp.vi.reg[VI_H_START_REG] = 0;

    Bus::rcp.mi.reg[MI_INTR_REG] &= ~(0x10 | 0x8 | 0x4 | 0x1);

    // copy boot code
    memcpy((uint8_t*)Bus::rcp.sp.dmem + 0x40, _bus->rom->getImage() + 0x40, 0xFC0);

    _reg[19].u = 0; // 0:Cart, 1:DD
    _reg[20].u = get_system_type_reg(_bus->rom->getSystemType());    // 0:PAL, 1:NTSC, 2:MPAL
    _reg[21].u = 0; // 0:ColdReset, 1:NMI
    _reg[22].u = get_cic_seed(_bus->rom->getCICChip());
    _reg[23].u = 0;

    uint32_t* sp_imem = Bus::rcp.sp.imem;

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
    LOG_INFO(Interpreter) << "Running...";
    _delay_slot = false;
    CoreControl::stop = false;
    Bus::state.skip_jump = 0;

    Bus::state.PC = Bus::state.last_jump_addr = 0xa4000040;
    Bus::state.next_interrupt = 624999;
    _bus->interrupt->initialize(_bus);

    while (!CoreControl::stop)
    {
        prefetch();

        (this->*instruction_table[_cur_instr.op])();
    }
}

void Interpreter::J(void)
{
    // DECLARE_JUMP(J,   (PC->f.j.inst_index<<2) | ((PCADDR+4) & 0xF0000000), 1, &reg[0],  0, 0)
    DO_JUMP(
        ((_cur_instr.target << 2) | (((uint32_t)Bus::state.PC + 4) & 0xF0000000)),
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
        ((_cur_instr.target << 2) | (((uint32_t)Bus::state.PC + 4) & 0xF0000000)),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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

    ++Bus::state.PC;
}

void Interpreter::ADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rs].s + signextend<int16_t, int32_t>(_cur_instr.immediate));
    }

    ++Bus::state.PC;
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
    
    ++Bus::state.PC;
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

    ++Bus::state.PC;
}

void Interpreter::ANDI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s & _cur_instr.immediate;
    
    ++Bus::state.PC;
}

void Interpreter::ORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s | _cur_instr.immediate;
    
    ++Bus::state.PC;
}

void Interpreter::XORI(void)
{
    _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s ^ _cur_instr.immediate;
    
    ++Bus::state.PC;
}

void Interpreter::LUI(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int16_t)_cur_instr.immediate << 16);
    }
    
    ++Bus::state.PC;
}

void Interpreter::SV(void)
{
    CoreControl::stop = true;
    LOG_ERROR(Interpreter) << "OP: " << std::hex << _cur_instr.code << "; Opcode " << _cur_instr.op << " reserved. Stopping...";
}

void Interpreter::BEQL(void)
{
    //DECLARE_JUMP(BEQL, PCADDR + (iimmediate + 1) * 4, irs == irt, &reg[0], 1, 0)
    DO_JUMP(
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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
        ((uint32_t)Bus::state.PC) + 4 + (signextend<int16_t, int32_t>(_cur_instr.immediate) << 2),
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

    ++Bus::state.PC;
}

void Interpreter::DADDIU(void)
{
    if (_cur_instr.rt)
    {
        _reg[_cur_instr.rt].s = _reg[_cur_instr.rs].s + signextend<int16_t, int64_t>(_cur_instr.immediate);
    }

    ++Bus::state.PC;
}

void Interpreter::LDL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~7;
    uint64_t value = 0;
    uint32_t offset = addr & 7;

    _bus->mem->readmem(masked_addr, &value, SIZE_DWORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (_reg[_cur_instr.rt].s & LDL_MASK[offset]);
        _reg[_cur_instr.rt].s += (value << LDL_SHIFT[offset]);
    }

    ++Bus::state.PC;
}

void Interpreter::LDR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~7;
    uint64_t value = 0;
    uint32_t offset = addr & 7;

    _bus->mem->readmem(masked_addr, &value, SIZE_DWORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (_reg[_cur_instr.rt].s & LDR_MASK[offset]);
        _reg[_cur_instr.rt].s += (value >> LDR_SHIFT[offset]);
    }

    ++Bus::state.PC;
}

void Interpreter::LB(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_BYTE);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int8_t, int64_t>((int8_t)_reg[_cur_instr.rt].s);
        }
    }

    ++Bus::state.PC;
}

void Interpreter::LH(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_HWORD);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int16_t, int64_t>((int16_t)_reg[_cur_instr.rt].s);
        }
    }

    ++Bus::state.PC;
}

void Interpreter::LWL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;

    _bus->mem->readmem(masked_addr, &value, SIZE_WORD);

    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (int32_t)(_reg[_cur_instr.rt].s & LWL_MASK[offset]);
        _reg[_cur_instr.rt].s += (int32_t)(value << LWL_SHIFT[offset]);
    }

    ++Bus::state.PC;
}

void Interpreter::LW(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_WORD);

        if (addr)
        {
            _reg[_cur_instr.rt].s = signextend<int32_t, int64_t>((int32_t)_reg[_cur_instr.rt].s);
        }
    }

    ++Bus::state.PC;
}

void Interpreter::LBU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
	
        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_BYTE);
    }

    ++Bus::state.PC;
}

void Interpreter::LHU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_HWORD);
    }

    ++Bus::state.PC;
}

void Interpreter::LWR(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;

    _bus->mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        _reg[_cur_instr.rt].s = (int32_t)(_reg[_cur_instr.rt].s & LWR_MASK[offset]);
        _reg[_cur_instr.rt].s += (int32_t)(value >> LWR_SHIFT[offset]);
    }

    ++Bus::state.PC;
}

void Interpreter::LWU(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_WORD);
    }

    ++Bus::state.PC;
}

void Interpreter::SB(void)
{
    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint8_t)(_reg[_cur_instr.rt].u & 0xFF), SIZE_BYTE);

    ++Bus::state.PC;
}

void Interpreter::SH(void)
{
    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint16_t)(_reg[_cur_instr.rt].u & 0xFFFF), SIZE_HWORD);

    ++Bus::state.PC;
}

void Interpreter::SWL(void)
{
    uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint32_t masked_addr = addr & ~3;
    uint64_t value = 0;
    uint32_t offset = addr & 3;

    _bus->mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        value &= SWL_MASK[offset];
        value += (uint32_t)(_reg[_cur_instr.rt].u >> SWL_SHIFT[offset]);

        _bus->mem->writemem(addr & ~0x03, value, SIZE_WORD);
    }

    ++Bus::state.PC;
}

void Interpreter::SW(void)
{
    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), (uint32_t)(_reg[_cur_instr.rt].u & 0xFFFFFFFF), SIZE_WORD);

    ++Bus::state.PC;
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

    _bus->mem->readmem(masked_addr, &value, SIZE_WORD);
    if (masked_addr)
    {
        value &= SWR_MASK[offset];
        value += (uint32_t)(_reg[_cur_instr.rt].u << SWR_SHIFT[offset]);

        _bus->mem->writemem(addr & ~0x03, value, SIZE_WORD);
    }

    ++Bus::state.PC;
}

void Interpreter::CACHE(void)
{
    ++Bus::state.PC;
}

void Interpreter::LL(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::LWC1(void)
{
    if (_cp0.COP1Unusable(*this))
        return;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));
    uint64_t dest = 0;

    _bus->mem->readmem(address, &dest, SIZE_WORD);

    if (address)
    {
        *((int32_t*)_s_reg[_cur_instr.ft]) = (int32_t)dest;
    }

    ++Bus::state.PC;
}

void Interpreter::LLD(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::LDC1(void)
{
    if (_cp0.COP1Unusable(*this))
        return;

    uint32_t address = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

    _bus->mem->readmem(address, (uint64_t*)_d_reg[_cur_instr.ft], SIZE_DWORD);

    ++Bus::state.PC;
}

void Interpreter::LD(void)
{
    if (_cur_instr.rt)
    {
        uint32_t addr = ((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset));

        _bus->mem->readmem(addr, &_reg[_cur_instr.rt].u, SIZE_DWORD);
    }

    ++Bus::state.PC;
}

void Interpreter::SC(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SWC1(void)
{
    if (_cp0.COP1Unusable(*this))
        return;

    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int32_t*)_s_reg[_cur_instr.ft]), SIZE_WORD);

    ++Bus::state.PC;
}

void Interpreter::SCD(void)
{
    NOT_IMPLEMENTED();
}

void Interpreter::SDC1(void)
{
    if (_cp0.COP1Unusable(*this))
        return;

    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), *((int64_t*)_d_reg[_cur_instr.ft]), SIZE_DWORD);

    ++Bus::state.PC;
}

void Interpreter::SD(void)
{
    _bus->mem->writemem(((uint32_t)_reg[_cur_instr.base].u + signextend<int16_t, int32_t>(_cur_instr.offset)), _reg[_cur_instr.rt].u, SIZE_DWORD);

    ++Bus::state.PC;
}

void Interpreter::genericIdle(uint32_t destination, bool take_jump, Register64* link, bool likely, bool cop1)
{
    int32_t skip;
    if (cop1 && _cp0.COP1Unusable(*this))
        return;

    if (take_jump)
    {
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
        skip = Bus::state.next_interrupt - Bus::state.cp0_reg[CP0_COUNT_REG];
        if (skip > 3)
        {
            Bus::state.cp0_reg[CP0_COUNT_REG] += (skip & 0xFFFFFFFC);
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
    if (cop1 && _cp0.COP1Unusable(*this))
        return;

    if (link != &_reg[0])
    {
        (*link).s = ((uint32_t)Bus::state.PC) + 8;
        (*link).s = signextend<int32_t, int64_t>((int32_t)(*link).s);
    }

    if (!likely || take_jump)
    {
        Bus::state.PC += 1;
        _delay_slot = true;

        prefetch();
        (this->*instruction_table[_cur_instr.op])();

        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
        _delay_slot = false;
        if (take_jump && !Bus::state.skip_jump)
        {
            Bus::state.PC = destination;
        }
    }
    else
    {
        Bus::state.PC += 2;
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
    }

    Bus::state.last_jump_addr = (uint32_t)Bus::state.PC;
    if (Bus::state.next_interrupt <= Bus::state.cp0_reg[CP0_COUNT_REG])
    {
        _bus->interrupt->generateInterrupt();
    }
}

void Interpreter::globalJump(uint32_t addr)
{
    Bus::state.PC = addr;
}

// NOTE: these exceptions assume that we never go into
// extended addressing mode, which is probably true for
// most if not all N64 games
void Interpreter::generalException(void)
{
    unsigned offset = 0x180;

    _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
    Bus::state.cp0_reg[CP0_STATUS_REG] |= 2;

    if (_delay_slot)
    {
        Bus::state.cp0_reg[CP0_CAUSE_REG] |= 0x80000000;
        Bus::state.cp0_reg[CP0_EPC_REG] = (uint32_t)Bus::state.PC - 4;
    }
    else
    {
        Bus::state.cp0_reg[CP0_CAUSE_REG] &= ~0x80000000U;
        Bus::state.cp0_reg[CP0_EPC_REG] = (uint32_t)Bus::state.PC;
    }

    if (Bus::state.cp0_reg[CP0_STATUS_REG] & 0x400000)
    {
        globalJump(0xBFC00200U + offset);
    }
    else
    {
        globalJump(0x80000000U + offset);
    }

    Bus::state.last_jump_addr = (uint32_t)Bus::state.PC;

    if (_delay_slot)
    {
        Bus::state.skip_jump = (uint32_t)Bus::state.PC;
        Bus::state.next_interrupt = 0;
    }
}

void Interpreter::TLBRefillException(unsigned int address, TLBProbeMode mode, bool miss)
{
    unsigned type, offset;

    if (mode != TLB_FAST_READ)
    {
        _cp0.updateCount(Bus::state.PC, _bus->rom->getCountPerOp());
    }

    if (mode == TLB_WRITE)
    {
        type = 0x3;
    }
    else
    {
        type = 0x2;
    }

    if (_delay_slot)
    {
        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & 0x2))
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] |= 0x80000000;
            Bus::state.cp0_reg[CP0_EPC_REG] = (uint32_t)Bus::state.PC - 4;

            // NOTE: offset should be 0x80 if we are in extended mode, but we don't implement it
            offset = 0x000;
        }
        else
        {
            offset = 0x180;
        }
    }
    else
    {
        if (!(Bus::state.cp0_reg[CP0_STATUS_REG] & 0x2))
        {
            Bus::state.cp0_reg[CP0_CAUSE_REG] &= ~0x80000000;
            Bus::state.cp0_reg[CP0_EPC_REG] = (uint32_t)Bus::state.PC;

            // NOTE: offset should be 0x80 if we are in extended mode, but we don't implement it
            offset = 0x000;
        }
        else
        {
            offset = 0x180;
        }
    }

    uint32_t vpn2 = address >> 13 & 0x7FFFF;
    uint8_t asid = Bus::state.cp0_reg[CP0_ENTRYHI_REG];

    uint64_t entryhi = (int32_t)((vpn2 << 13) | asid);

    uint64_t context = Bus::state.cp0_reg[CP0_CONTEXT_REG];
    context &= ~(0x7FFFFULL << 4);
    context |= vpn2 << 4;

    Bus::state.cp0_reg[CP0_ENTRYHI_REG] = entryhi;
    Bus::state.cp0_reg[CP0_CONTEXT_REG] = context;
    Bus::state.cp0_reg[CP0_BADVADDR_REG] = address;

    if (!miss)
    {
        offset = 0x180;
    }

    Bus::state.cp0_reg[CP0_STATUS_REG] |= 0x2;
    Bus::state.cp0_reg[CP0_CAUSE_REG] = (Bus::state.cp0_reg[CP0_CAUSE_REG] & ~0xFF) | (type << 2);

    // HACK: Subtract 4 from these addresses because TLB exceptions are only raised
    // during the exection of an instruction where PC gets incremented, which means
    // that by the time we get back to the fetch, the PC will be 1 slot further
    // than what we set here. This doesn't apply to general exceptions above
    // because general exceptions are only emulated at jumps, where the PC is not
    // further changed at the end of execution.
    if (Bus::state.cp0_reg[CP0_STATUS_REG] & 0x400000)
    {
        globalJump(0xBFC00200U + offset - 4);
    }
    else
    {
        globalJump(0x80000000U + offset - 4);
    }

    Bus::state.last_jump_addr = (uint32_t)Bus::state.PC + 4;

    if (_delay_slot)
    {
        Bus::state.skip_jump = (uint32_t)Bus::state.PC + 4;
        Bus::state.next_interrupt = 0;
    }
}
