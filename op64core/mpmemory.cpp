#include <algorithm>

#include "logger.h"
#include "util.h"

#include "mpmemory.h"
#include "rom.h"
#include "icpu.h"
#include "interrupthandler.h"
#include "cp0.h"
#include "dma.h"
#include "pif.h"
#include "flashram.h"
#include "plugins.h"
#include "gfxplugin.h"
#include "audioplugin.h"
#include "rspplugin.h"


#define MEM_NOT_IMPLEMENTED() \
    Bus::stop = true; \
    LOG_ERROR("OP: %x; Function %s in %s line %i not implemented. Stopping...\n", Bus::cur_instr->code, __func__, __FILE__, __LINE__);


static FrameBufferInfo fbInfo[6];
static char framebufferRead[0x800];

void MPMemory::initialize(void)
{
    IMemory::initialize();

    fill_array(readmem_table, 0, 0x10000, &MPMemory::read_nomem);
    fill_array(writemem_table, 0, 0x10000, &MPMemory::write_nomem);

    // init rdram
    fill_array(_rdram, 0, 0x800000 / 4, 0);

    fill_array(readmem_table, 0x8000, 0x80, &MPMemory::read_rdram);
    fill_array(readmem_table, 0xa000, 0x80, &MPMemory::read_rdram);
    fill_array(writemem_table, 0x8000, 0x80, &MPMemory::write_rdram);
    fill_array(writemem_table, 0xa000, 0x80, &MPMemory::write_rdram);

    fill_array(readmem_table, 0x8080, 0x370, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa080, 0x370, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8080, 0x370, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa080, 0x370, &MPMemory::write_nothing);

    // rdram reg
    readmem_table[0x83f0] = &MPMemory::read_rdram_reg;
    readmem_table[0xa3f0] = &MPMemory::read_rdram_reg;
    writemem_table[0x83f0] = &MPMemory::write_rdram_reg;
    writemem_table[0xa3f0] = &MPMemory::write_rdram_reg;
    fill_array(_rdram_reg, 0, RDRAM_NUM_REGS, 0);
    fill_array(readrdram_table, 0, 0x10000, &trash);
    readrdram_table[0x0] = &_rdram_reg[RDRAM_CONFIG_REG];
    readrdram_table[0x4] = &_rdram_reg[RDRAM_DEVICE_ID_REG];
    readrdram_table[0x8] = &_rdram_reg[RDRAM_DELAY_REG];
    readrdram_table[0xc] = &_rdram_reg[RDRAM_MODE_REG];
    readrdram_table[0x10] = &_rdram_reg[RDRAM_REF_INTERVAL_REG];
    readrdram_table[0x14] = &_rdram_reg[RDRAM_REF_ROW_REG];
    readrdram_table[0x18] = &_rdram_reg[RDRAM_RAS_INTERVAL_REG];
    readrdram_table[0x1c] = &_rdram_reg[RDRAM_MIN_INTERVAL_REG];
    readrdram_table[0x20] = &_rdram_reg[RDRAM_ADDR_SELECT_REG];
    readrdram_table[0x24] = &_rdram_reg[RDRAM_DEVICE_MANUF_REG];


    fill_array(readmem_table, 0x83f1, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa3f1, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x83f1, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa3f1, 0xf, &MPMemory::write_nothing);


    // sp mem
    readmem_table[0x8400] = &MPMemory::read_rsp_mem;
    readmem_table[0xa400] = &MPMemory::read_rsp_mem;
    writemem_table[0x8400] = &MPMemory::write_rsp_mem;
    writemem_table[0xa400] = &MPMemory::write_rsp_mem;

    fill_array(_SP_DMEM, 0, (0x1000 / 4), 0);
    fill_array(_SP_IMEM, 0, (0x1000 / 4), 0);

    fill_array(readmem_table, 0x8401, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa401, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8401, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa401, 0x3, &MPMemory::write_nothing);

    //sp reg
    readmem_table[0x8404] = &MPMemory::read_rsp_reg;
    readmem_table[0xa404] = &MPMemory::read_rsp_reg;
    writemem_table[0x8404] = &MPMemory::write_rsp_reg;
    writemem_table[0xa404] = &MPMemory::write_rsp_reg;
    fill_array(_sp_reg, 0, SP_NUM_REGS, 0);
    _sp_reg[SP_STATUS_REG] = 1;
    fill_array(readsp_table, 0, 0x10000, &trash);
    readsp_table[0x0] = &_sp_reg[SP_MEM_ADDR_REG];
    readsp_table[0x4] = &_sp_reg[SP_DRAM_ADDR_REG];
    readsp_table[0x8] = &_sp_reg[SP_RD_LEN_REG];
    readsp_table[0xc] = &_sp_reg[SP_WR_LEN_REG];
    readsp_table[0x10] = &_sp_reg[SP_STATUS_REG];
    readsp_table[0x14] = &_sp_reg[SP_DMA_FULL_REG];
    readsp_table[0x18] = &_sp_reg[SP_DMA_BUSY_REG];
    readsp_table[0x1c] = &_sp_reg[SP_SEMAPHORE_REG];

    fill_array(readmem_table, 0x8405, 0x3, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa405, 0x3, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8405, 0x3, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa405, 0x3, &MPMemory::write_nothing);

    readmem_table[0x8408] = &MPMemory::read_rsp_stat;
    readmem_table[0xa408] = &MPMemory::read_rsp_stat;
    writemem_table[0x8408] = &MPMemory::write_rsp_stat;
    writemem_table[0xa408] = &MPMemory::write_rsp_stat;
    fill_array(readsp_stat_table, 0, 0x10000, &trash);
    readsp_stat_table[0x0] = &_sp_reg[SP_PC_REG];
    readsp_stat_table[0x4] = &_sp_reg[SP_IBIST_REG];

    fill_array(readmem_table, 0x8409, 0x7, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa409, 0x7, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8409, 0x7, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa409, 0x7, &MPMemory::write_nothing);

    // dp reg
    readmem_table[0x8410] = &MPMemory::read_dp;
    readmem_table[0xa410] = &MPMemory::read_dp;
    writemem_table[0x8410] = &MPMemory::write_dp;
    writemem_table[0xa410] = &MPMemory::write_dp;
    fill_array(_dp_reg, 0, DPC_NUM_REGS, 0);
    fill_array(readdp_table, 0, 0x10000, &trash);
    readdp_table[0x0] = &_dp_reg[DPC_START_REG];
    readdp_table[0x4] = &_dp_reg[DPC_END_REG];
    readdp_table[0x8] = &_dp_reg[DPC_CURRENT_REG];
    readdp_table[0xc] = &_dp_reg[DPC_STATUS_REG];
    readdp_table[0x10] = &_dp_reg[DPC_CLOCK_REG];
    readdp_table[0x14] = &_dp_reg[DPC_BUFBUSY_REG];
    readdp_table[0x18] = &_dp_reg[DPC_PIPEBUSY_REG];
    readdp_table[0x1c] = &_dp_reg[DPC_TMEM_REG];

    fill_array(readmem_table, 0x8411, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa411, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8411, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa411, 0xf, &MPMemory::write_nothing);

    // dps reg
    readmem_table[0x8420] = &MPMemory::read_dps;
    readmem_table[0xa420] = &MPMemory::read_dps;
    writemem_table[0x8420] = &MPMemory::write_dps;
    writemem_table[0xa420] = &MPMemory::write_dps;
    fill_array(_dps_reg, 0, DPS_NUM_REGS, 0);
    fill_array(readdps_table, 0, 0x10000, &trash);
    readdps_table[0x0] = &_dps_reg[DPS_TBIST_REG];
    readdps_table[0x4] = &_dps_reg[DPS_TEST_MODE_REG];
    readdps_table[0x8] = &_dps_reg[DPS_BUFTEST_ADDR_REG];
    readdps_table[0xc] = &_dps_reg[DPS_BUFTEST_DATA_REG];

    fill_array(readmem_table, 0x8421, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa421, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8421, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa421, 0xf, &MPMemory::write_nothing);


    // mi reg
    readmem_table[0xa830] = &MPMemory::read_mi;
    readmem_table[0xa430] = &MPMemory::read_mi;
    writemem_table[0xa830] = &MPMemory::write_mi;
    writemem_table[0xa430] = &MPMemory::write_mi;
    fill_array(_mi_reg, 0, MI_NUM_REGS, 0);
    _mi_reg[MI_VERSION_REG] = 0x02020102;
    fill_array(readmi_table, 0, 0x10000, &trash);
    readmi_table[0x0] = &_mi_reg[MI_INIT_MODE_REG];
    readmi_table[0x4] = &_mi_reg[MI_VERSION_REG];
    readmi_table[0x8] = &_mi_reg[MI_INTR_REG];
    readmi_table[0xc] = &_mi_reg[MI_INTR_MASK_REG];

    fill_array(readmem_table, 0x8431, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa431, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8431, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa431, 0xf, &MPMemory::write_nothing);

    // vi reg
    readmem_table[0x8440] = &MPMemory::read_vi;
    readmem_table[0xa440] = &MPMemory::read_vi;
    writemem_table[0x8440] = &MPMemory::write_vi;
    writemem_table[0xa440] = &MPMemory::write_vi;
    fill_array(_vi_reg, 0, VI_NUM_REGS, 0);
    fill_array(readvi_table, 0, 0x10000, &trash);
    readvi_table[0x0] = &_vi_reg[VI_STATUS_REG];
    readvi_table[0x4] = &_vi_reg[VI_ORIGIN_REG];
    readvi_table[0x8] = &_vi_reg[VI_WIDTH_REG];
    readvi_table[0xc] = &_vi_reg[VI_INTR_REG];
    readvi_table[0x10] = &_vi_reg[VI_CURRENT_REG];
    readvi_table[0x14] = &_vi_reg[VI_BURST_REG];
    readvi_table[0x18] = &_vi_reg[VI_V_SYNC_REG];
    readvi_table[0x1c] = &_vi_reg[VI_H_SYNC_REG];
    readvi_table[0x20] = &_vi_reg[VI_LEAP_REG];
    readvi_table[0x24] = &_vi_reg[VI_H_START_REG];
    readvi_table[0x28] = &_vi_reg[VI_V_START_REG];
    readvi_table[0x2c] = &_vi_reg[VI_V_BURST_REG];
    readvi_table[0x30] = &_vi_reg[VI_X_SCALE_REG];
    readvi_table[0x34] = &_vi_reg[VI_Y_SCALE_REG];

    fill_array(readmem_table, 0x8441, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa441, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8441, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa441, 0xf, &MPMemory::write_nothing);


    // ai reg
    readmem_table[0x8450] = &MPMemory::read_ai;
    readmem_table[0xa450] = &MPMemory::read_ai;
    writemem_table[0x8450] = &MPMemory::write_ai;
    writemem_table[0xa450] = &MPMemory::write_ai;
    fill_array(_ai_reg, 0, AI_NUM_REGS, 0);
    fill_array(readai_table, 0, 0x10000, &trash);
    readai_table[0x0] = &_ai_reg[AI_DRAM_ADDR_REG];
    readai_table[0x4] = &_ai_reg[AI_LEN_REG];
    readai_table[0x8] = &_ai_reg[AI_CONTROL_REG];
    readai_table[0xc] = &_ai_reg[AI_STATUS_REG];
    readai_table[0x10] = &_ai_reg[AI_DACRATE_REG];
    readai_table[0x14] = &_ai_reg[AI_BITRATE_REG];

    fill_array(readmem_table, 0x8451, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa451, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8451, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa451, 0xf, &MPMemory::write_nothing);


    // pi reg
    readmem_table[0x8460] = &MPMemory::read_pi;
    readmem_table[0xa460] = &MPMemory::read_pi;
    writemem_table[0x8460] = &MPMemory::write_pi;
    writemem_table[0xa460] = &MPMemory::write_pi;
    fill_array(_pi_reg, 0, PI_NUM_REGS, 0);
    fill_array(readpi_table, 0, 0x10000, &trash);
    readpi_table[0x0] = &_pi_reg[PI_DRAM_ADDR_REG];
    readpi_table[0x4] = &_pi_reg[PI_CART_ADDR_REG];
    readpi_table[0x8] = &_pi_reg[PI_RD_LEN_REG];
    readpi_table[0xc] = &_pi_reg[PI_WR_LEN_REG];
    readpi_table[0x10] = &_pi_reg[PI_STATUS_REG];
    readpi_table[0x14] = &_pi_reg[PI_BSD_DOM1_LAT_REG];
    readpi_table[0x18] = &_pi_reg[PI_BSD_DOM1_PWD_REG];
    readpi_table[0x1c] = &_pi_reg[PI_BSD_DOM1_PGS_REG];
    readpi_table[0x20] = &_pi_reg[PI_BSD_DOM1_RLS_REG];
    readpi_table[0x24] = &_pi_reg[PI_BSD_DOM2_LAT_REG];
    readpi_table[0x28] = &_pi_reg[PI_BSD_DOM2_PWD_REG];
    readpi_table[0x2c] = &_pi_reg[PI_BSD_DOM2_PGS_REG];
    readpi_table[0x30] = &_pi_reg[PI_BSD_DOM2_RLS_REG];

    fill_array(readmem_table, 0x8461, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa461, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8461, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa461, 0xf, &MPMemory::write_nothing);


    // ri reg
    readmem_table[0x8470] = &MPMemory::read_ri;
    readmem_table[0xa470] = &MPMemory::read_ri;
    writemem_table[0x8470] = &MPMemory::write_ri;
    writemem_table[0xa470] = &MPMemory::write_ri;
    fill_array(_ri_reg, 0, RI_NUM_REGS, 0);
    fill_array(readri_table, 0, 0x10000, &trash);
    readri_table[0x0] = &_ri_reg[RI_MODE_REG];
    readri_table[0x4] = &_ri_reg[RI_CONFIG_REG];
    readri_table[0x8] = &_ri_reg[RI_CURRENT_LOAD_REG];
    readri_table[0xc] = &_ri_reg[RI_SELECT_REG];
    readri_table[0x10] = &_ri_reg[RI_REFRESH_REG];
    readri_table[0x14] = &_ri_reg[RI_LATENCY_REG];
    readri_table[0x18] = &_ri_reg[RI_RERROR_REG];
    readri_table[0x1c] = &_ri_reg[RI_WERROR_REG];

    fill_array(readmem_table, 0x8471, 0xf, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa471, 0xf, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8471, 0xf, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa471, 0xf, &MPMemory::write_nothing);


    // si reg
    readmem_table[0x8480] = &MPMemory::read_si;
    readmem_table[0xa480] = &MPMemory::read_si;
    writemem_table[0x8480] = &MPMemory::write_si;
    writemem_table[0xa480] = &MPMemory::write_si;
    fill_array(_si_reg, 0, SI_NUM_REGS, 0);
    fill_array(readsi_table, 0, 0x10000, &trash);
    readsi_table[0x0] = &_si_reg[SI_DRAM_ADDR_REG];
    readsi_table[0x4] = &_si_reg[SI_PIF_ADDR_RD64B_REG];
    readsi_table[0x8] = &trash;
    readsi_table[0x10] = &_si_reg[SI_PIF_ADDR_WR64B_REG];
    readsi_table[0x14] = &trash;
    readsi_table[0x18] = &_si_reg[SI_STATUS_REG];

    fill_array(readmem_table, 0x8481, 0x37f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa481, 0x37f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8481, 0x37f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa481, 0x37f, &MPMemory::write_nothing);


    // flashram
    readmem_table[0x8800] = &MPMemory::read_flashram_status;
    readmem_table[0xa800] = &MPMemory::read_flashram_status;
    readmem_table[0x8801] = &MPMemory::read_nothing;
    readmem_table[0xa801] = &MPMemory::read_nothing;
    writemem_table[0x8800] = &MPMemory::write_flashram_dummy;
    writemem_table[0xa800] = &MPMemory::write_flashram_dummy;
    writemem_table[0x8801] = &MPMemory::write_flashram_command;
    writemem_table[0xa801] = &MPMemory::write_flashram_command;

    fill_array(readmem_table, 0x8802, 0x7fe, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xa802, 0x7fe, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x8802, 0x7fe, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xa802, 0x7fe, &MPMemory::write_nothing);


    // rom
    uint32_t rom_size = Bus::rom->getSize();
    fill_array(readmem_table, 0x9000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(readmem_table, 0xb000, (rom_size >> 16), &MPMemory::read_rom);
    fill_array(writemem_table, 0x9000, (rom_size >> 16), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000, (rom_size >> 16), &MPMemory::write_rom);

    fill_array(readmem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(readmem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9000 + (rom_size >> 16), 0x9fc0 - (0x9000 + (rom_size >> 16)), &MPMemory::write_nothing);
    fill_array(writemem_table, 0xb000 + (rom_size >> 16), 0xbfc0 - (0xb000 + (rom_size >> 16)), &MPMemory::write_nothing);


    // pif
    readmem_table[0x9fc0] = &MPMemory::read_pif;
    readmem_table[0xbfc0] = &MPMemory::read_pif;
    writemem_table[0x9fc0] = &MPMemory::write_pif;
    writemem_table[0xbfc0] = &MPMemory::write_pif;
    fill_array(_PIF_RAM, 0, (0x40 / 4), 0);

    fill_array(readmem_table, 0x9fc1, 0x3f, &MPMemory::read_nothing);
    fill_array(readmem_table, 0xbfc1, 0x3f, &MPMemory::read_nothing);
    fill_array(writemem_table, 0x9fc1, 0x3f, &MPMemory::write_nothing);
    fill_array(writemem_table, 0xbfc1, 0x3f, &MPMemory::write_nothing);

    fbInfo[0].addr = 0;
}

void MPMemory::read_nothing(uint32_t& address, uint64_t* dest, DataSize size)
{
    if (size == SIZE_WORD && address == 0xa5000508)
        *dest = 0xFFFFFFFF;
    else
        *dest = 0;
}

void MPMemory::read_nomem(uint32_t& address, uint64_t* dest, DataSize size)
{
    address = TLB::virtual_to_physical_address(address, 0);
    if (address == 0x00000000)
        return;

    readmem(address, dest, size);
}

void MPMemory::read_rdram(uint32_t& address, uint64_t* dest, DataSize size)
{
    switch (size)
    {
    case SIZE_WORD:
        *dest = *((uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF)));
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*(uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF))) << 32) |
            ((*(uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF) + 4)));
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)(Bus::rdram8 + (HES(address & 0xFFFFFF))));
        break;
    case SIZE_BYTE:
        *dest = *(Bus::rdram8 + (BES(address & 0xFFFFFF)));
        break;
    }
}

void MPMemory::read_rdram_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readrdram_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readrdram_table[addr_low]) << 32) |
            *readrdram_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readrdram_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readrdram_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        break;
    }
}

void MPMemory::read_rsp_mem(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    uint8_t* SP_DMEM = Bus::sp_dmem8;
    uint8_t* SP_IMEM = Bus::sp_imem8;

    if (addr_low < 0x1000)
    {
        switch (size)
        {
        case SIZE_WORD:
            *dest = *((uint32_t*)(SP_DMEM + (addr_low)));
            break;
        case SIZE_BYTE:
            *dest = *(SP_DMEM + (BES(addr_low)));
            break;
        case SIZE_HWORD:
            *dest = *((uint16_t*)(SP_DMEM + HES(addr_low)));
            break;
        case SIZE_DWORD:
            *dest = ((uint64_t)(*(uint32_t*)(SP_DMEM + (addr_low))) << 32) |
                ((*(uint32_t*)(SP_DMEM + addr_low + 4)));
            break;
        }
    }
    else if (addr_low < 0x2000)
    {
        switch (size)
        {
        case SIZE_WORD:
            *dest = *((uint32_t*)(SP_IMEM + (addr_low & 0xfff)));
            break;
        case SIZE_BYTE:
            *dest = *(SP_IMEM + (BES(addr_low & 0xfff)));
            break;
        case SIZE_HWORD:
            *dest = *((uint16_t*)(SP_IMEM + (HES(addr_low & 0xfff))));
            break;
        case SIZE_DWORD:
            *dest = ((uint64_t)(*(uint32_t*)(SP_IMEM + ((addr_low & 0xfff)))) << 32) |
                ((*(uint32_t*)(SP_IMEM + (addr_low & 0xfff) + 4)));
            break;
        }
    }
    else
    {
        read_nomem(address, dest, size);
    }
}

void MPMemory::read_rsp_reg(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        *dest = *(readsp_table[addr_low]);
        switch (addr_low)
        {
        case 0x1c:
            _sp_reg[SP_SEMAPHORE_REG] = 1;
            break;
        }
    }
        break;
    case SIZE_DWORD:
    {
        *dest = ((uint64_t)(*readsp_table[addr_low]) << 32) |
            *readsp_table[addr_low + 4];
        switch (addr_low)
        {
        case 0x18:
            _sp_reg[SP_SEMAPHORE_REG] = 1;
            break;
        }
    }
        break;
    case SIZE_HWORD:
    {
        *dest = *((uint16_t*)((uint8_t*)readsp_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        switch (addr_low)
        {
        case 0x1c:
        case 0x1e:
            _sp_reg[SP_SEMAPHORE_REG] = 1;
            break;
        }
    }
        break;
    case SIZE_BYTE:
    {
        *dest = *((uint8_t*)readsp_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        switch (addr_low)
        {
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x1f:
            _sp_reg[SP_SEMAPHORE_REG] = 1;
            break;
        }
    }
        break;
    }
}

void MPMemory::read_rsp_stat(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readsp_stat_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readsp_stat_table[addr_low]) << 32) |
            *readsp_stat_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readsp_stat_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readsp_stat_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        break;
    }
}

void MPMemory::read_dp(uint32_t& address, uint64_t* dest, DataSize size)
{
    MEM_NOT_IMPLEMENTED();
}

void MPMemory::read_dps(uint32_t& address, uint64_t* dest, DataSize size)
{
    MEM_NOT_IMPLEMENTED();
}

void MPMemory::read_mi(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readmi_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readmi_table[addr_low]) << 32) |
            *readmi_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readmi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readmi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        break;
    default:
        break;
    }
}

void MPMemory::read_vi(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x10:
            Bus::cpu->getcp0()->update_count();
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[_VI_DELAY] - (*(Bus::next_vi) - Bus::cp0_reg[CP0_COUNT_REG])) / 1500;
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[VI_CURRENT_REG]&(~1)) | *(Bus::vi_field);
            break;
        }
        *dest = *(readvi_table[addr_low]);
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x10:
            Bus::cpu->getcp0()->update_count();
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[_VI_DELAY] - (*(Bus::next_vi) - Bus::cp0_reg[CP0_COUNT_REG])) / 1500;
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[VI_CURRENT_REG]&(~1)) | *(Bus::vi_field);
            break;
        }
        *dest = ((uint64_t)(*readvi_table[addr_low]) << 32) |
            *readvi_table[addr_low + 4];
    }
        break;
    case SIZE_HWORD:
    {
        switch (addr_low)
        {
        case 0x10:
        case 0x12:
            Bus::cpu->getcp0()->update_count();
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[_VI_DELAY] - (*(Bus::next_vi) - Bus::cp0_reg[CP0_COUNT_REG])) / 1500;
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[VI_CURRENT_REG]&(~1)) | *(Bus::vi_field);
            break;
        }
        *dest = *((uint16_t*)((uint8_t*)readvi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
    }
        break;
    case SIZE_BYTE:
    {
        switch (addr_low)
        {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            Bus::cpu->getcp0()->update_count();
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[_VI_DELAY] - (*(Bus::next_vi) - Bus::cp0_reg[CP0_COUNT_REG])) / 1500;
            _vi_reg[VI_CURRENT_REG] = (_vi_reg[VI_CURRENT_REG] & (~1)) | *(Bus::vi_field);
            break;
        }
        *dest = *((uint8_t*)readvi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
    }
        break;
    }
}

void MPMemory::read_ai(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x4:
            Bus::cpu->getcp0()->update_count();
            if (_ai_reg[_AI_CURRENT_DELAY] != 0 && Bus::interrupt->find_event(AI_INT) != 0 && (Bus::interrupt->find_event(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG]) < 0x80000000)
                *dest = ((Bus::interrupt->find_event(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG])*(int64_t)_ai_reg[_AI_CURRENT_LEN]) / _ai_reg[_AI_CURRENT_DELAY];
            else
                *dest = 0;
            return;
            break;
        }
        *dest = *(readai_table[addr_low]);
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x0:
            Bus::cpu->getcp0()->update_count();
            if (_ai_reg[_AI_CURRENT_DELAY] != 0 && Bus::interrupt->find_event(AI_INT) != 0)
                *dest = ((Bus::interrupt->find_event(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG])*(int64_t)_ai_reg[_AI_CURRENT_LEN]) / _ai_reg[_AI_CURRENT_DELAY];
            else
                *dest = 0;
            *dest |= (uint64_t)_ai_reg[AI_DRAM_ADDR_REG] << 32;
            return;
            break;
        }
        *dest = ((uint64_t)(*readai_table[addr_low]) << 32) |
            *readai_table[addr_low + 4];
    }
        break;
    case SIZE_HWORD:
    {
        uint32_t len;
        switch (addr_low)
        {
        case 0x4:
        case 0x6:
            Bus::cpu->getcp0()->update_count();
            if (_ai_reg[_AI_CURRENT_DELAY] != 0 && Bus::interrupt->find_event(AI_INT) != 0)
                len = (uint32_t)(((Bus::interrupt->find_event(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG]) * (int64_t)_ai_reg[_AI_CURRENT_LEN]) / _ai_reg[_AI_CURRENT_DELAY]);
            else
                len = 0;
            *dest = *((uint16_t*)((uint8_t*)&len
                + (HES(addr_low & 3))));
            return;
            break;
        }
        *dest = *((uint16_t*)((uint8_t*)readai_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
    }
        break;
    case SIZE_BYTE:
    {
        uint32_t len;
        switch (addr_low)
        {
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            Bus::cpu->getcp0()->update_count();
            if (_ai_reg[_AI_CURRENT_DELAY] != 0 && Bus::interrupt->find_event(AI_INT) != 0)
                len = (uint32_t)(((Bus::interrupt->find_event(AI_INT) - Bus::cp0_reg[CP0_COUNT_REG]) * (int64_t)_ai_reg[_AI_CURRENT_LEN]) / _ai_reg[_AI_CURRENT_DELAY]);
            else
                len = 0;
            *dest = *((uint8_t*)&len + (BES(addr_low & 3)));
            return;
            break;
        }
        *dest = *((uint8_t*)readai_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
    }
        break;
    }
}

void MPMemory::read_pi(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readpi_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readpi_table[addr_low]) << 32) |
            *readpi_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readpi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readpi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        break;
    }
}

void MPMemory::read_ri(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readri_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readri_table[addr_low]) << 32) |
            *readri_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readri_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readri_table[addr_low & 0xfffc] + (BES(addr_low & 3)));
        break;
    }
}

void MPMemory::read_si(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *dest = *(readsi_table[addr_low]);
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*readsi_table[addr_low]) << 32) |
            *readsi_table[addr_low + 4];
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)((uint8_t*)readsi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3))));
        break;
    case SIZE_BYTE:
        *dest = *((uint8_t*)readsi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3)));
        break;
    }
}

void MPMemory::read_flashram_status(uint32_t& address, uint64_t* dest, DataSize size)
{
    if (size != SIZE_WORD)
    {
        LOG_ERROR("Reading flashram status as non-word");
        return;
    }

    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        *dest = Bus::flashram->readFlashStatus();
    }
    else
    {
        LOG_ERROR("Reading flashram command with non-flashram save type");
    }
}

void MPMemory::read_rom(uint32_t& address, uint64_t* dest, DataSize size)
{
    switch (size)
    {
    case SIZE_WORD:
    {
        if (_rom_lastwrite)
        {
            *dest = _rom_lastwrite;
            _rom_lastwrite = 0;
        }
        else
        {
            *dest = *((uint32_t*)(Bus::rom_image + (address & 0x03FFFFFF)));
        }
    }
        break;
    case SIZE_DWORD:
        *dest = ((uint64_t)(*((uint32_t*)(Bus::rom_image + (address & 0x03FFFFFF)))) << 32) |
            *((uint32_t*)(Bus::rom_image + ((address + 4) & 0x03FFFFFF)));
        break;
    case SIZE_HWORD:
        *dest = *((uint16_t*)(Bus::rom_image + (HES(address) & 0x03FFFFFF)));
        break;
    case SIZE_BYTE:
        *dest = *(Bus::rom_image + (BES(address) & 0x03FFFFFF));
        break;
    }
}

void MPMemory::read_pif(uint32_t& address, uint64_t* dest, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    if ((addr_low > 0x7FF) || (addr_low < 0x7C0))
    {
        LOG_ERROR("reading a byte in PIF at invalid address 0x%x", address);
        *dest = 0;
        return;
    }

    switch (size)
    {
    case SIZE_WORD:
    {
        *dest = byteswap_u32(*((uint32_t*)(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0)));
    }
        break;
    case SIZE_DWORD:
    {
        *dest = ((uint64_t)byteswap_u32(*((uint32_t*)(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0))) << 32) |
            byteswap_u32(*((uint32_t*)(Bus::pif_ram8 + ((address + 4) & 0x7FF) - 0x7C0)));
    }
        break;
    case SIZE_HWORD:
    {
        *dest = (*(Bus::pif_ram8 + ((address & 0x7FF) - 0x7C0)) << 8) |
            *(Bus::pif_ram8 + (((address + 1) & 0x7FF) - 0x7C0));
    }
        break;
    case SIZE_BYTE:
    {
        *dest = *(Bus::pif_ram8 + ((address & 0x7FF) - 0x7C0));
    }
        break;
    }
}


void MPMemory::read_rdramFB(uint32_t& address, uint64_t* dest, DataSize size)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        if (fbInfo[i].addr)
        {
            uint32_t start = fbInfo[i].addr & 0x7FFFFF;
            uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
            if ((address & 0x7FFFFF) >= start && (address & 0x7FFFFF) <= end &&
                framebufferRead[(address & 0x7FFFFF) >> 12])
            {
                Bus::plugins->gfx()->fbRead(address);
                framebufferRead[(address & 0x7FFFFF) >> 12] = 0;
            }
        }
    }

    read_rdram(address, dest, size);
}


void MPMemory::write_nomem(uint32_t address, uint64_t src, DataSize size)
{
    MEM_NOT_IMPLEMENTED();
}

void MPMemory::write_rdram(uint32_t address, uint64_t src, DataSize size)
{
    switch (size)
    {
    case SIZE_WORD:
        *((uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF))) = (uint32_t)src;
        break;
    case SIZE_DWORD:
        *((uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF))) = (uint32_t)(src >> 32);
        *((uint32_t*)(Bus::rdram8 + (address & 0xFFFFFF) + 4)) = (uint32_t)(src & 0xFFFFFFFF);
        break;
    case SIZE_HWORD:
        *(uint16_t*)((Bus::rdram8 + (HES(address & 0xFFFFFF)))) = (uint16_t)src;
        break;
    case SIZE_BYTE:
        *((Bus::rdram8 + (BES(address & 0xFFFFFF)))) = (uint8_t)src;
        break;
    }
}

void MPMemory::write_nothing(uint32_t address, uint64_t src, DataSize size)
{
    // does nothing
}

void MPMemory::write_rdram_reg(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *readrdram_table[addr_low] = (uint32_t)src;
        break;
    case SIZE_DWORD:
        *readrdram_table[addr_low] = (uint32_t)(src >> 32);
        *readrdram_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
        break;
    case SIZE_HWORD:
        *((uint16_t*)((uint8_t*)readrdram_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
        break;
    case SIZE_BYTE:
        *((uint8_t*)readrdram_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
        break;
    }
}

void MPMemory::write_rsp_mem(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    uint8_t* SP_DMEM = Bus::sp_dmem8;
    uint8_t* SP_IMEM = Bus::sp_imem8;

    if (addr_low < 0x1000)
    {
        switch (size)
        {
        case SIZE_WORD:
            *((uint32_t*)(SP_DMEM + (addr_low))) = (uint32_t)src;
            break;
        case SIZE_BYTE:
            *(SP_DMEM + BES(addr_low)) = (uint8_t)src;
            break;
        case SIZE_HWORD:
            *((uint16_t*)(SP_DMEM + HES(addr_low))) = (uint16_t)src;
            break;
        case SIZE_DWORD:
            *((uint32_t*)(SP_DMEM + addr_low)) = (uint32_t)(src >> 32);
            *((uint32_t*)(SP_DMEM + addr_low + 4)) = (uint32_t)(src & 0xFFFFFFFF);
            break;
        }
    }
    else if (addr_low < 0x2000)
    {
        switch (size)
        {
        case SIZE_WORD:
            *((uint32_t*)(SP_IMEM + (addr_low & 0xfff))) = (uint32_t)src;
            break;
        case SIZE_BYTE:
            *(SP_IMEM + (BES(addr_low & 0xfff))) = (uint8_t)src;
            break;
        case SIZE_HWORD:
            *((uint16_t*)(SP_IMEM + (HES(addr_low & 0xfff)))) = (uint16_t)src;
            break;
        case SIZE_DWORD:
            *((uint32_t*)(SP_IMEM + (addr_low & 0xfff))) = (uint32_t)(src >> 32);
            *((uint32_t*)(SP_IMEM + (addr_low & 0xfff) + 4)) = (uint32_t)(src & 0xFFFFFFFF);
            break;
        }
    }
    else
    {
        write_nomem(address, src, size);
    }
}

void MPMemory::write_rsp_reg(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x10:
            _sp_reg[_SP_WRITE_STATUS_REG] = (uint32_t)src;
            update_sp_reg();
        case 0x14:
        case 0x18:
            return;
            break;
        }

        *readsp_table[addr_low] = (uint32_t)src;

        switch (addr_low)
        {
        case 0x8:
            DMA::writeSP();
            break;
        case 0xc:
            DMA::readSP();
            break;
        case 0x1c:
            _sp_reg[SP_SEMAPHORE_REG] = 0;
            break;
        }
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x10:
            _sp_reg[_SP_WRITE_STATUS_REG] = (uint32_t)(src >> 32);
            update_sp_reg();
            return;
            break;
        case 0x18:
            _sp_reg[SP_SEMAPHORE_REG] = 0;
            return;
            break;
        }

        *readsp_table[addr_low] = (uint32_t)(src >> 32);
        *readsp_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);

        switch (addr_low)
        {
        case 0x8:
            DMA::writeSP();
            DMA::readSP();
            break;
        }
    }
        break;
    case SIZE_HWORD:
    {
        switch (addr_low)
        {
        case 0x10:
        case 0x12:
            *((uint16_t*)((uint8_t*)&_sp_reg[_SP_WRITE_STATUS_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
        case 0x14:
        case 0x16:
        case 0x18:
        case 0x1a:
            return;
            break;
        }

        *((uint16_t*)((uint8_t*)readsp_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;

        switch (addr_low)
        {
        case 0x8:
        case 0xa:
            DMA::writeSP();
            break;
        case 0xc:
        case 0xe:
            DMA::readSP();
            break;
        case 0x1c:
        case 0x1e:
            _sp_reg[SP_SEMAPHORE_REG] = 0;
            break;
        }
    }
        break;
    case SIZE_BYTE:
    {
        switch (addr_low)
        {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            *((uint8_t*)&_sp_reg[_SP_WRITE_STATUS_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
            return;
            break;
        }

        *((uint8_t*)readsp_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;

        switch (addr_low)
        {
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
            DMA::writeSP();
            break;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            DMA::readSP();
            break;
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x1f:
            _sp_reg[SP_SEMAPHORE_REG] = 0;
            break;
        }
    }
        break;
    }
}

void MPMemory::write_rsp_stat(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *readsp_stat_table[addr_low] = (uint32_t)src;
        break;
    case SIZE_DWORD:
        *readsp_stat_table[addr_low] = (uint32_t)(src >> 32);
        *readsp_stat_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
        break;
    case SIZE_HWORD:
        *((uint16_t*)((uint8_t*)readsp_stat_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
        break;
    case SIZE_BYTE:
        *((uint8_t*)readsp_stat_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
        break;
    }
}

void MPMemory::write_dp(uint32_t address, uint64_t src, DataSize size)
{
    MEM_NOT_IMPLEMENTED();
}

void MPMemory::write_dps(uint32_t address, uint64_t src, DataSize size)
{
    MEM_NOT_IMPLEMENTED();
}

void MPMemory::write_mi(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x0:
            _mi_reg[_MI_WRITE_INIT_MODE_REG] = (uint32_t)src;
            update_MI_init_mode_reg();
            break;
        case 0xc:
            _mi_reg[_MI_WRITE_INTR_MASK_REG] = (uint32_t)src;
            update_MI_intr_mask_reg();

            Bus::interrupt->check_interrupt();
            Bus::cpu->getcp0()->update_count();

            if (*(Bus::next_interrupt) <= Bus::cp0_reg[CP0_COUNT_REG])
                Bus::interrupt->gen_interrupt();

            break;
        }
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x0:
            _mi_reg[_MI_WRITE_INIT_MODE_REG] = (uint32_t)(src >> 32);
            update_MI_init_mode_reg();
            break;
        case 0x8:
            _mi_reg[_MI_WRITE_INTR_MASK_REG] = (uint32_t)(src & 0xFFFFFFFF);
            update_MI_intr_mask_reg();

            Bus::interrupt->check_interrupt();
            Bus::cpu->getcp0()->update_count();

            if (*(Bus::next_interrupt) <= Bus::cp0_reg[CP0_COUNT_REG])
                Bus::interrupt->gen_interrupt();
            break;
        }
    }
        break;
    case SIZE_HWORD:
    {
        switch (addr_low)
        {
        case 0x0:
        case 0x2:
            *((uint16_t*)((uint8_t*)&_mi_reg[_MI_WRITE_INIT_MODE_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            update_MI_init_mode_reg();
            break;
        case 0xc:
        case 0xe:
            *((uint16_t*)((uint8_t*)&_mi_reg[_MI_WRITE_INTR_MASK_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            update_MI_intr_mask_reg();

            Bus::interrupt->check_interrupt();
            Bus::cpu->getcp0()->update_count();

            if (*(Bus::next_interrupt) <= Bus::cp0_reg[CP0_COUNT_REG])
                Bus::interrupt->gen_interrupt();
            break;
        }
    }
        break;
    case SIZE_BYTE:
    {
        switch (addr_low)
        {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
            *((uint8_t*)&_mi_reg[_MI_WRITE_INIT_MODE_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            update_MI_init_mode_reg();
            break;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            *((uint8_t*)&_mi_reg[_MI_WRITE_INTR_MASK_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            update_MI_intr_mask_reg();

            Bus::interrupt->check_interrupt();
            Bus::cpu->getcp0()->update_count();

            if (*(Bus::next_interrupt) <= Bus::cp0_reg[CP0_COUNT_REG])
                Bus::interrupt->gen_interrupt();
            break;
        }
    }
        break;
    }
}

void MPMemory::write_vi(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x0:
            if (_vi_reg[VI_STATUS_REG] != (uint32_t)src)
            {
                _vi_reg[VI_STATUS_REG] = (uint32_t)src;
                if (Bus::plugins->gfx()->ViStatusChanged != nullptr) { Bus::plugins->gfx()->ViStatusChanged(); }
            }
            return;
            break;
        case 0x8:
            if (_vi_reg[VI_WIDTH_REG] != (uint32_t)src)
            {
                _vi_reg[VI_WIDTH_REG] = (uint32_t)src;
                if (Bus::plugins->gfx()->ViWidthChanged != nullptr) { Bus::plugins->gfx()->ViWidthChanged(); }
            }
            return;
            break;
        case 0x10:
            _mi_reg[MI_INTR_REG] &= ~0x8;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
        *readvi_table[addr_low] = (uint32_t)src;
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x0:
            if (_vi_reg[VI_STATUS_REG] != (uint32_t)(src >> 32))
            {
                _vi_reg[VI_STATUS_REG] = ((uint32_t)(src >> 32));
                if (Bus::plugins->gfx()->ViStatusChanged != nullptr) { Bus::plugins->gfx()->ViStatusChanged(); }
            }
            _vi_reg[VI_ORIGIN_REG] = (uint32_t)(src & 0xFFFFFFFF);
            return;
            break;
        case 0x8:
            if (_vi_reg[VI_WIDTH_REG] != (uint32_t)(src >> 32))
            {
                _vi_reg[VI_WIDTH_REG] = ((uint32_t)(src >> 32));
                if (Bus::plugins->gfx()->ViWidthChanged != nullptr) { Bus::plugins->gfx()->ViWidthChanged(); }
            }
            _vi_reg[VI_INTR_REG] = (uint32_t)(src & 0xFFFFFFFF);
            return;
            break;
        case 0x10:
            _mi_reg[MI_INTR_REG] &= ~0x8;
            Bus::interrupt->check_interrupt();
            _vi_reg[VI_BURST_REG] = (uint32_t)(src & 0xFFFFFFFF);
            return;
            break;
        }
        *readvi_table[addr_low] = (uint32_t)(src >> 32);
        *readvi_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
    }
        break;
    case SIZE_HWORD:
    {
        int temp;
        switch (addr_low)
        {
        case 0x0:
        case 0x2:
            temp = _vi_reg[VI_STATUS_REG];
            *((uint16_t*)((uint8_t*)&temp
                + (HES(addr_low & 3)))) = (uint16_t)src;
            if (_vi_reg[VI_STATUS_REG] != temp)
            {
                _vi_reg[VI_STATUS_REG] = (temp);
                if (Bus::plugins->gfx()->ViStatusChanged != nullptr) { Bus::plugins->gfx()->ViStatusChanged(); }
            }
            return;
            break;
        case 0x8:
        case 0xa:
            temp = _vi_reg[VI_STATUS_REG];
            *((uint16_t*)((uint8_t*)&temp
                + (HES(addr_low & 3)))) = (uint16_t)src;
            if (_vi_reg[VI_WIDTH_REG] != temp)
            {
                _vi_reg[VI_WIDTH_REG] = (temp);
                if (Bus::plugins->gfx()->ViWidthChanged != nullptr) { Bus::plugins->gfx()->ViWidthChanged(); }
            }
            return;
            break;
        case 0x10:
        case 0x12:
            _mi_reg[MI_INTR_REG] &= ~0x8;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
        *((uint16_t*)((uint8_t*)readvi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
    }
        break;
    case SIZE_BYTE:
    {
        int temp;
        switch (addr_low)
        {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
            temp = _vi_reg[VI_STATUS_REG];
            *((uint8_t*)&temp
                + (BES(addr_low & 3))) = (uint8_t)src;
            if (_vi_reg[VI_STATUS_REG] != temp)
            {
                _vi_reg[VI_STATUS_REG] = (temp);
                if (Bus::plugins->gfx()->ViStatusChanged != nullptr) { Bus::plugins->gfx()->ViStatusChanged(); }
            }
            return;
            break;
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
            temp = _vi_reg[VI_STATUS_REG];
            *((uint8_t*)&temp
                + (BES(addr_low & 3))) = (uint8_t)src;
            if (_vi_reg[VI_WIDTH_REG] != temp)
            {
                _vi_reg[VI_WIDTH_REG] = (temp);
                if (Bus::plugins->gfx()->ViWidthChanged != nullptr) { Bus::plugins->gfx()->ViWidthChanged(); }
            }
            return;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            _mi_reg[MI_INTR_REG] &= ~0x8;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
        *((uint8_t*)readvi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
    }
        break;
    }
}

void MPMemory::write_ai(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        uint32_t freq, delay = 0;
        switch (addr_low)
        {
        case 0x4:
            _ai_reg[AI_LEN_REG] = (uint32_t)src;
            if (Bus::plugins->audio()->LenChanged != nullptr) { Bus::plugins->audio()->LenChanged(); }

            freq = Bus::rom->getAiDACRate() / (_ai_reg[AI_DACRATE_REG] + 1);
            if (freq)
                delay = (uint32_t)(((uint64_t)_ai_reg[AI_LEN_REG] * _vi_reg[_VI_DELAY] * Bus::rom->getViLimit()) / (freq * 4));

            if (_ai_reg[AI_STATUS_REG] & 0x40000000) // busy
            {
                _ai_reg[_AI_NEXT_DELAY] = delay;
                _ai_reg[_AI_NEXT_LEN] = _ai_reg[AI_LEN_REG];
                _ai_reg[AI_STATUS_REG] |= 0x80000000;
            }
            else
            {
                _ai_reg[_AI_CURRENT_DELAY] = delay;
                _ai_reg[_AI_CURRENT_LEN] = _ai_reg[AI_LEN_REG];
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(AI_INT, delay);
                _ai_reg[AI_STATUS_REG] |= 0x40000000;
            }
            return;
            break;
        case 0xc:
            _mi_reg[MI_INTR_REG] &= ~0x4;
            Bus::interrupt->check_interrupt();
            return;
            break;
        case 0x10:
            if (_ai_reg[AI_DACRATE_REG] != (uint32_t)src)
            {
                _ai_reg[AI_DACRATE_REG] = (uint32_t)src;
                Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
            }
            return;
            break;
        }
        *readai_table[addr_low] = (uint32_t)src;
    }
        break;
    case SIZE_DWORD:
    {
        uint32_t delay = 0;
        switch (addr_low)
        {
        case 0x0:
            _ai_reg[AI_DRAM_ADDR_REG] = (uint32_t)(src >> 32);
            _ai_reg[AI_LEN_REG] = (uint32_t)(src & 0xFFFFFFFF);
            if (Bus::plugins->audio()->LenChanged != nullptr) { Bus::plugins->audio()->LenChanged(); }

            delay = (uint32_t)(((uint64_t)_ai_reg[AI_LEN_REG] * (_ai_reg[AI_DACRATE_REG] + 1) *
                _vi_reg[_VI_DELAY] * Bus::rom->getViLimit()) / Bus::rom->getAiDACRate());

            if (_ai_reg[AI_STATUS_REG] & 0x40000000) // busy
            {
                _ai_reg[_AI_NEXT_DELAY] = delay;
                _ai_reg[_AI_NEXT_LEN] = _ai_reg[AI_LEN_REG];
                _ai_reg[AI_STATUS_REG] |= 0x80000000;
            }
            else
            {
                _ai_reg[_AI_CURRENT_DELAY] = delay;
                _ai_reg[_AI_CURRENT_LEN] = _ai_reg[AI_LEN_REG];
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(AI_INT, delay / 2);
                _ai_reg[AI_STATUS_REG] |= 0x40000000;
            }
            return;
            break;
        case 0x8:
            _ai_reg[AI_CONTROL_REG] = (uint32_t)(src >> 32);
            _mi_reg[MI_INTR_REG] &= ~0x4;
            Bus::interrupt->check_interrupt();
            return;
            break;
        case 0x10:
            if (_ai_reg[AI_DACRATE_REG] != (uint32_t)(src >> 32))
            {
                _ai_reg[AI_DACRATE_REG] = (uint32_t)(src >> 32);
                Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
            }
            _ai_reg[AI_BITRATE_REG] = (uint32_t)(src & 0xFFFFFFFF);
            return;
            break;
        }
        *readai_table[addr_low] = (uint32_t)(src >> 32);
        *readai_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
    }
        break;
    case SIZE_HWORD:
    {
        int32_t temp;
        uint32_t delay = 0;
        switch (addr_low)
        {
        case 0x4:
        case 0x6:
            temp = _ai_reg[AI_LEN_REG];
            *((uint16_t*)((uint8_t*)&temp
                + (HES(addr_low & 3)))) = (uint16_t)src;
            _ai_reg[AI_LEN_REG] = temp;
            if (Bus::plugins->audio()->LenChanged != nullptr) { Bus::plugins->audio()->LenChanged(); }

            delay = (uint32_t)(((uint64_t)_ai_reg[AI_LEN_REG]*(_ai_reg[AI_DACRATE_REG] + 1)*
                _vi_reg[_VI_DELAY] * Bus::rom->getViLimit()) / Bus::rom->getAiDACRate());

            if (_ai_reg[AI_STATUS_REG] & 0x40000000) // busy
            {
                _ai_reg[_AI_NEXT_DELAY] = delay;
                _ai_reg[_AI_NEXT_LEN] = _ai_reg[AI_LEN_REG];
                _ai_reg[AI_STATUS_REG] |= 0x80000000;
            }
            else
            {
                _ai_reg[_AI_CURRENT_DELAY] = delay;
                _ai_reg[_AI_CURRENT_LEN] = _ai_reg[AI_LEN_REG];
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(AI_INT, delay / 2);
                _ai_reg[AI_STATUS_REG] |= 0x40000000;
            }
            return;
            break;
        case 0xc:
        case 0xe:
            _mi_reg[MI_INTR_REG] &= ~0x4;
            Bus::interrupt->check_interrupt();
            return;
            break;
        case 0x10:
        case 0x12:
            temp = _ai_reg[AI_DACRATE_REG];
            *((uint16_t*)((uint8_t*)&temp
                + (HES(addr_low & 3)))) = (uint16_t)src;
            if (_ai_reg[AI_DACRATE_REG] != temp)
            {
                _ai_reg[AI_DACRATE_REG] = temp;
                Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
            }
            return;
            break;
        }
        *((uint16_t*)((uint8_t*)readai_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
    }
        break;
    case SIZE_BYTE:
    {
        int temp;
        uint32_t delay = 0;
        switch (addr_low)
        {
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            temp = _ai_reg[AI_LEN_REG];
            *((uint8_t*)&temp
                + (BES(addr_low & 3))) = (uint8_t)src;
            _ai_reg[AI_LEN_REG] = temp;
            if (Bus::plugins->audio()->LenChanged != nullptr) { Bus::plugins->audio()->LenChanged(); }

            delay = (uint32_t)(((uint64_t)_ai_reg[AI_LEN_REG]*(_ai_reg[AI_DACRATE_REG] + 1)*
                _vi_reg[_VI_DELAY]*Bus::rom->getViLimit()) / Bus::rom->getAiDACRate());
            //delay = 0;

            if (_ai_reg[AI_STATUS_REG] & 0x40000000) // busy
            {
                _ai_reg[_AI_NEXT_DELAY] = delay;
                _ai_reg[_AI_NEXT_LEN] = _ai_reg[AI_LEN_REG];
                _ai_reg[AI_STATUS_REG] |= 0x80000000;
            }
            else
            {
                _ai_reg[_AI_CURRENT_DELAY] = delay;
                _ai_reg[_AI_CURRENT_LEN] = _ai_reg[AI_LEN_REG];
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(AI_INT, delay / 2);
                _ai_reg[AI_STATUS_REG] |= 0x40000000;
            }
            return;
            break;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            _mi_reg[MI_INTR_REG] &= ~0x4;
            Bus::interrupt->check_interrupt();
            return;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            temp = _ai_reg[AI_DACRATE_REG];
            *((uint8_t*)&temp
                + (BES(addr_low & 3))) = (uint8_t)src;
            if (_ai_reg[AI_DACRATE_REG] != temp)
            {
                _ai_reg[AI_DACRATE_REG] = temp;
                Bus::plugins->audio()->DacrateChanged(Bus::rom->getSystemType());
            }
            return;
            break;
        }
        *((uint8_t*)readai_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
    }
        break;
    }
}

void MPMemory::write_pi(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x8:
            _pi_reg[PI_RD_LEN_REG] = (uint32_t)src;
            DMA::readPI();
            return;
            break;
        case 0xc:
            _pi_reg[PI_WR_LEN_REG] = (uint32_t)src;
            DMA::writePI();
            return;
            break;
        case 0x10:
            if (((uint32_t)src) & 2)
            {
                _mi_reg[MI_INTR_REG] &= ~0x10;
                Bus::interrupt->check_interrupt();
            }
            return;
            break;
        case 0x14:
        case 0x18:
        case 0x1c:
        case 0x20:
        case 0x24:
        case 0x28:
        case 0x2c:
        case 0x30:
            *readpi_table[addr_low] = ((uint32_t)src) & 0xFF;
            return;
            break;
        }

        *readpi_table[addr_low] = (uint32_t)src;
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x8:
            _pi_reg[PI_RD_LEN_REG] = (uint32_t)(src >> 32);
            DMA::readPI();
            _pi_reg[PI_WR_LEN_REG] = (uint32_t)(src & 0xFFFFFFFF);
            DMA::writePI();
            return;
            break;
        case 0x10:
            if ((uint32_t)src)
            {
                _mi_reg[MI_INTR_REG] &= ~0x10;
                Bus::interrupt->check_interrupt();
            }
            *readpi_table[addr_low + 4] = (uint32_t)(src & 0xFF);
            return;
            break;
        case 0x18:
        case 0x20:
        case 0x28:
        case 0x30:
            *readpi_table[addr_low] = (uint32_t)(src >> 32) & 0xFF;
            *readpi_table[addr_low + 4] = (uint32_t)(src & 0xFF);
            return;
            break;
        }

        *readpi_table[addr_low] = (uint32_t)(src >> 32);
        *readpi_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
    }
        break;
    case SIZE_HWORD:
    {
        switch (addr_low)
        {
        case 0x8:
        case 0xa:
            *((uint16_t*)((uint8_t*)&_pi_reg[PI_RD_LEN_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            DMA::readPI();
            return;
            break;
        case 0xc:
        case 0xe:
            *((uint16_t*)((uint8_t*)&_pi_reg[PI_WR_LEN_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            DMA::writePI();
            return;
            break;
        case 0x10:
        case 0x12:
            if ((uint32_t)src)
            {
                _mi_reg[MI_INTR_REG] &= ~0x10;
                Bus::interrupt->check_interrupt();
            }
            return;
            break;
        case 0x16:
        case 0x1a:
        case 0x1e:
        case 0x22:
        case 0x26:
        case 0x2a:
        case 0x2e:
        case 0x32:
            *((uint16_t*)((uint8_t*)readpi_table[addr_low & 0xfffc]
                + (HES(addr_low & 3)))) = ((uint16_t)src) & 0xFF;
            return;
            break;
        case 0x14:
        case 0x18:
        case 0x1c:
        case 0x20:
        case 0x24:
        case 0x28:
        case 0x2c:
        case 0x30:
            return;
            break;
        }

        *((uint16_t*)((uint8_t*)readpi_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
    }
        break;
    case SIZE_BYTE:
    {
        switch (addr_low)
        {
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
            *((uint8_t*)&_pi_reg[PI_RD_LEN_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            DMA::readPI();
            return;
            break;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            *((uint8_t*)&_pi_reg[PI_WR_LEN_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            DMA::writePI();
            return;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            if ((uint32_t)src)
            {
                _mi_reg[MI_INTR_REG] &= ~0x10;
                Bus::interrupt->check_interrupt();
            }
            return;
            break;
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1c:
        case 0x1d:
        case 0x1e:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2c:
        case 0x2d:
        case 0x2e:
        case 0x30:
        case 0x31:
        case 0x32:
            return;
            break;
        }

        *((uint8_t*)readpi_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
    }
        break;
    }
}

void MPMemory::write_ri(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
        *readri_table[addr_low] = (uint32_t)src;
        break;
    case SIZE_DWORD:
        *readri_table[addr_low] = (uint32_t)(src >> 32);
        *readri_table[addr_low + 4] = (uint32_t)(src & 0xFFFFFFFF);
        break;
    case SIZE_HWORD:
        *((uint16_t*)((uint8_t*)readri_table[addr_low & 0xfffc]
            + (HES(addr_low & 3)))) = (uint16_t)src;
        break;
    case SIZE_BYTE:
        *((uint8_t*)readri_table[addr_low & 0xfffc]
            + (BES(addr_low & 3))) = (uint8_t)src;
        break;
    }
}

void MPMemory::write_si(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        switch (addr_low)
        {
        case 0x0:
            _si_reg[SI_DRAM_ADDR_REG] = (uint32_t)src;
            return;
            break;
        case 0x4:
            _si_reg[SI_PIF_ADDR_RD64B_REG] = (uint32_t)src;
            DMA::readSI();
            return;
            break;
        case 0x10:
            _si_reg[SI_PIF_ADDR_WR64B_REG] = (uint32_t)src;
            DMA::writeSI();
            return;
            break;
        case 0x18:
            _mi_reg[MI_INTR_REG] &= ~0x2;
            _si_reg[SI_STATUS_REG] &= ~0x1000;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
    }
        break;
    case SIZE_DWORD:
    {
        switch (addr_low)
        {
        case 0x0:
            _si_reg[SI_DRAM_ADDR_REG] = (uint32_t)(src >> 32);
            _si_reg[SI_PIF_ADDR_RD64B_REG] = (uint32_t)(src & 0xFFFFFFFF);
            DMA::readSI();
            return;
            break;
        case 0x10:
            _si_reg[SI_PIF_ADDR_WR64B_REG] = (uint32_t)(src >> 32);
            DMA::writeSI();
            return;
            break;
        case 0x18:
            _mi_reg[MI_INTR_REG] &= ~0x2;
            _si_reg[SI_STATUS_REG] &= ~0x1000;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
    }
        break;
    case SIZE_HWORD:
    {
        switch (addr_low)
        {
        case 0x0:
        case 0x2:
            *((uint16_t*)((uint8_t*)&_si_reg[SI_DRAM_ADDR_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            return;
            break;
        case 0x4:
        case 0x6:
            *((uint16_t*)((uint8_t*)&_si_reg[SI_PIF_ADDR_RD64B_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            DMA::readSI();
            return;
            break;
        case 0x10:
        case 0x12:
            *((uint16_t*)((uint8_t*)&_si_reg[SI_PIF_ADDR_WR64B_REG]
                + (HES(addr_low & 3)))) = (uint16_t)src;
            DMA::writeSI();
            return;
            break;
        case 0x18:
        case 0x1a:
            _mi_reg[MI_INTR_REG] &= ~0x2;
            _si_reg[SI_STATUS_REG] &= ~0x1000;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
    }
        break;
    case SIZE_BYTE:
    {
        switch (addr_low)
        {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
            *((uint8_t*)&_si_reg[SI_DRAM_ADDR_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            return;
            break;
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            *((uint8_t*)&_si_reg[SI_PIF_ADDR_RD64B_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            DMA::readSI();
            return;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            *((uint8_t*)&_si_reg[SI_PIF_ADDR_WR64B_REG]
                + (BES(addr_low & 3))) = (uint8_t)src;
            DMA::writeSI();
            return;
            break;
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
            _mi_reg[MI_INTR_REG] &= ~0x2;
            _si_reg[SI_STATUS_REG] &= ~0x1000;
            Bus::interrupt->check_interrupt();
            return;
            break;
        }
    }
        break;
    }
}

void MPMemory::write_flashram_dummy(uint32_t address, uint64_t src, DataSize size)
{
}

void MPMemory::write_flashram_command(uint32_t address, uint64_t src, DataSize size)
{
    if (size != SIZE_WORD)
    {
        LOG_ERROR("Writing flashram command as non-word");
        return;
    }

    if (Bus::rom->getSaveType() == SAVETYPE_AUTO)
    {
        Bus::rom->setSaveType(SAVETYPE_FLASH_RAM);
    }

    if (Bus::rom->getSaveType() == SAVETYPE_FLASH_RAM && !(address & 0xffff))
    {
        Bus::flashram->writeFlashCommand((uint32_t)src);
    }
    else
    {
        LOG_ERROR("Writing flashram command with non-flashram save type");
    }
}

void MPMemory::write_rom(uint32_t address, uint64_t src, DataSize size)
{
    if (size == SIZE_WORD)
    {
        _rom_lastwrite = (uint32_t)src;
    }
}

void MPMemory::write_pif(uint32_t address, uint64_t src, DataSize size)
{
    uint32_t addr_low = address & 0xffff;

    switch (size)
    {
    case SIZE_WORD:
    {
        if ((addr_low > 0x7FF) || (addr_low < 0x7C0))
        {
            LOG_ERROR("writing a word in PIF at invalid address 0x%x", address);
            return;
        }

        *((uint32_t*)(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0)) = byteswap_u32((uint32_t)src);
        if ((address & 0x7FF) == 0x7FC)
        {
            if (Bus::pif_ram8[0x3F] == 0x08)
            {
                Bus::pif_ram8[0x3F] = 0;
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
            }
            else
            {
                Bus::pif->pifWrite();
            }
        }
    }
        break;
    case SIZE_DWORD:
    {
        if ((addr_low > 0x7FF) || (addr_low < 0x7C0))
        {
            LOG_ERROR("writing a double word in PIF at 0x%x", address);
            return;
        }

        *((uint32_t*)(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0)) =
            byteswap_u32((uint32_t)(src >> 32));
        *((uint32_t*)(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0)) = // WTF: shouldn't this be address + 4?????
            byteswap_u32((uint32_t)(src & 0xFFFFFFFF));
        if ((address & 0x7FF) == 0x7F8)
        {
            if (Bus::pif_ram8[0x3F] == 0x08)
            {
                Bus::pif_ram8[0x3F] = 0;
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
            }
            else
            {
                Bus::pif->pifWrite();
            }
        }
    }
        break;
    case SIZE_HWORD:
    {
        if ((addr_low > 0x7FF) || (addr_low < 0x7C0))
        {
            LOG_ERROR("writing a hword in PIF at invalid address 0x%x", address);
            return;
        }

        *(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0) = ((uint16_t)src) >> 8;
        *(Bus::pif_ram8 + ((address + 1) & 0x7FF) - 0x7C0) = ((uint16_t)src) & 0xFF;
        if ((address & 0x7FF) == 0x7FE)
        {
            if (Bus::pif_ram8[0x3F] == 0x08)
            {
                Bus::pif_ram8[0x3F] = 0;
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
            }
            else
                Bus::pif->pifWrite();
        }
    }
        break;
    case SIZE_BYTE:
    {
        if ((addr_low > 0x7FF) || (addr_low < 0x7C0))
        {
            LOG_ERROR("writing a byte in PIF at invalid address 0x%x", address);
            return;
        }

        *(Bus::pif_ram8 + (address & 0x7FF) - 0x7C0) = (uint8_t)src;
        if ((address & 0x7FF) == 0x7FF)
        {
            if (Bus::pif_ram8[0x3F] == 0x08)
            {
                Bus::pif_ram8[0x3F] = 0;
                Bus::cpu->getcp0()->update_count();
                Bus::interrupt->add_interrupt_event(SI_INT, /*0x100*/0x900);
            }
            else
                Bus::pif->pifWrite();
        }
    }
        break;
    }
}


void MPMemory::write_rdramFB(uint32_t address, uint64_t src, DataSize size)
{
    for (uint32_t i = 0; i < 6; i++)
    {
        if (fbInfo[i].addr)
        {
            uint32_t start = fbInfo[i].addr & 0x7FFFFF;
            uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
            if ((address & 0x7FFFFF) >= start && (address & 0x7FFFFF) <= end)
            {
                switch (size)
                {
                case SIZE_WORD:
                    Bus::plugins->gfx()->fbWrite(address, 4);
                    break;
                case SIZE_DWORD:
                    Bus::plugins->gfx()->fbWrite(address, 8);
                    break;
                case SIZE_HWORD:
                    Bus::plugins->gfx()->fbWrite(HES(address), 2);
                    break;
                case SIZE_BYTE:
                    Bus::plugins->gfx()->fbWrite(BES(address), 1);
                    break;
                }
            }
        }
    }

    write_rdram(address, src, size);
}


void MPMemory::update_MI_init_mode_reg(void)
{
    _mi_reg[MI_INIT_MODE_REG] &= ~0x7F; // init_length
    _mi_reg[MI_INIT_MODE_REG] |= _mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x7F;

    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x80) // clear init_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x80;
    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x100) // set init_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x80;

    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x200) // clear ebus_test_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x100;
    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x400) // set ebus_test_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x100;

    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x800) // clear DP interrupt
    {
        _mi_reg[MI_INTR_REG] &= ~0x20;
        Bus::interrupt->check_interrupt();
    }

    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x1000) // clear RDRAM_reg_mode
        _mi_reg[MI_INIT_MODE_REG] &= ~0x200;
    if (_mi_reg[_MI_WRITE_INIT_MODE_REG] & 0x2000) // set RDRAM_reg_mode
        _mi_reg[MI_INIT_MODE_REG] |= 0x200;
}


void MPMemory::update_MI_intr_mask_reg(void)
{
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x1)   _mi_reg[MI_INTR_MASK_REG] &= ~0x1; // clear SP mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x2)   _mi_reg[MI_INTR_MASK_REG] |= 0x1; // set SP mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x4)   _mi_reg[MI_INTR_MASK_REG] &= ~0x2; // clear SI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x8)   _mi_reg[MI_INTR_MASK_REG] |= 0x2; // set SI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x10)  _mi_reg[MI_INTR_MASK_REG] &= ~0x4; // clear AI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x20)  _mi_reg[MI_INTR_MASK_REG] |= 0x4; // set AI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x40)  _mi_reg[MI_INTR_MASK_REG] &= ~0x8; // clear VI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x80)  _mi_reg[MI_INTR_MASK_REG] |= 0x8; // set VI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x100) _mi_reg[MI_INTR_MASK_REG] &= ~0x10; // clear PI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x200) _mi_reg[MI_INTR_MASK_REG] |= 0x10; // set PI mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x400) _mi_reg[MI_INTR_MASK_REG] &= ~0x20; // clear DP mask
    if (_mi_reg[_MI_WRITE_INTR_MASK_REG] & 0x800) _mi_reg[MI_INTR_MASK_REG] |= 0x20; // set DP mask
}


void MPMemory::update_sp_reg(void)
{
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x1) // clear halt
        _sp_reg[SP_STATUS_REG] &= ~0x1;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x2) // set halt
        _sp_reg[SP_STATUS_REG] |= 0x1;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x4) // clear broke
        _sp_reg[SP_STATUS_REG] &= ~0x2;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x8) // clear SP interrupt
    {
        _mi_reg[MI_INTR_REG] &= ~1;
        Bus::interrupt->check_interrupt();
    }

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x10) // set SP interrupt
    {
        _mi_reg[MI_INTR_REG] |= 1;
        Bus::interrupt->check_interrupt();
    }

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x20) // clear single step
        _sp_reg[SP_STATUS_REG] &= ~0x20;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x40) // set single step
        _sp_reg[SP_STATUS_REG] |= 0x20;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x80) // clear interrupt on break
        _sp_reg[SP_STATUS_REG] &= ~0x40;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x100) // set interrupt on break
        _sp_reg[SP_STATUS_REG] |= 0x40;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x200) // clear signal 0
        _sp_reg[SP_STATUS_REG] &= ~0x80;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x400) // set signal 0
        _sp_reg[SP_STATUS_REG] |= 0x80;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x800) // clear signal 1
        _sp_reg[SP_STATUS_REG] &= ~0x100;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x1000) // set signal 1
        _sp_reg[SP_STATUS_REG] |= 0x100;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x2000) // clear signal 2
        _sp_reg[SP_STATUS_REG] &= ~0x200;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x4000) // set signal 2
        _sp_reg[SP_STATUS_REG] |= 0x200;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x8000) // clear signal 3
        _sp_reg[SP_STATUS_REG] &= ~0x400;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x10000) // set signal 3
        _sp_reg[SP_STATUS_REG] |= 0x400;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x20000) // clear signal 4
        _sp_reg[SP_STATUS_REG] &= ~0x800;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x40000) // set signal 4
        _sp_reg[SP_STATUS_REG] |= 0x800;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x80000) // clear signal 5
        _sp_reg[SP_STATUS_REG] &= ~0x1000;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x100000) // set signal 5
        _sp_reg[SP_STATUS_REG] |= 0x1000;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x200000) // clear signal 6
        _sp_reg[SP_STATUS_REG] &= ~0x2000;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x400000) // set signal 6
        _sp_reg[SP_STATUS_REG] |= 0x2000;

    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x800000) // clear signal 7
        _sp_reg[SP_STATUS_REG] &= ~0x4000;
    if (_sp_reg[_SP_WRITE_STATUS_REG] & 0x1000000) // set signal 7
        _sp_reg[SP_STATUS_REG] |= 0x4000;

    //if (get_event(SP_INT)) return;
    if (!(_sp_reg[_SP_WRITE_STATUS_REG] & 0x1) &&
        !(_sp_reg[_SP_WRITE_STATUS_REG] & 0x4)) 
        return;

    if (!(_sp_reg[SP_STATUS_REG] & 0x3)) // !halt && !broke
    {
        prepare_rsp();
    }
}



void MPMemory::prepare_rsp(void)
{
    int32_t save_pc = _sp_reg[SP_PC_REG] & ~0xFFF;

    // Video task
    if (_SP_DMEM[0xFC0 / 4] == 1)
    {
        if (_dp_reg[DPC_STATUS_REG] & 0x2) // DP frozen (DK64, BC)
        {
            // don't do the task now
            // the task will be done when DP is unfreezed (see update_DPC)
            return;
        }

        // unprotecting old frame buffers
        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite &&
            fbInfo[0].addr)
        {
            vec_for (uint32_t i = 0; i < 6; i++)
            {
                if (fbInfo[i].addr)
                {
                    uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                    uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                    start = start >> 16;
                    end = end >> 16;

                    fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdram);
                    fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdram);
                    fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdram);
                    fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdram);
                }
            }
        }

        _sp_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xffffffff);
        _sp_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getcp0()->update_count();

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->add_interrupt_event(SP_INT, 1000);
        }

        if (_mi_reg[MI_INTR_REG] & 0x20)
        {
            Bus::interrupt->add_interrupt_event(DP_INT, 1000);
        }

        _mi_reg[MI_INTR_REG] &= ~0x21;
        _sp_reg[SP_STATUS_REG] &= ~0x303;

        // protecting new frame buffers
        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite)
        {
            Bus::plugins->gfx()->fbGetInfo(fbInfo);
        }

        if (Bus::plugins->gfx()->fbGetInfo && Bus::plugins->gfx()->fbRead && Bus::plugins->gfx()->fbWrite
            && fbInfo[0].addr)
        {
            vec_for (uint32_t i = 0; i < 6; i++)
            {
                if (fbInfo[i].addr)
                {
                    uint32_t j;
                    uint32_t start = fbInfo[i].addr & 0x7FFFFF;
                    uint32_t end = start + fbInfo[i].width * fbInfo[i].height * fbInfo[i].size - 1;
                    uint32_t start1 = start;
                    uint32_t end1 = end;

                    start >>= 16;
                    end >>= 16;

                    fill_array(readmem_table, 0x8000 + start, end - start + 1, &MPMemory::read_rdramFB);
                    fill_array(readmem_table, 0xa000 + start, end - start + 1, &MPMemory::read_rdramFB);
                    fill_array(writemem_table, 0x8000 + start, end - start + 1, &MPMemory::write_rdramFB);
                    fill_array(writemem_table, 0xa000 + start, end - start + 1, &MPMemory::write_rdramFB);

                    start <<= 4;
                    end <<= 4;
                    vec_for (j = start; j <= end; j++)
                    {
                        if (j >= start1 && j <= end1)
                        {
                            framebufferRead[j] = 1;
                        }
                        else
                        {
                            framebufferRead[j] = 0;
                        }
                    }
                }
            }
        }
    }
    // Audio task
    else if (_SP_DMEM[0xFC0 / 4] == 2)
    {
        _sp_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        _sp_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getcp0()->update_count();

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->add_interrupt_event(SP_INT, 4000/*500*/);
        }

        _mi_reg[MI_INTR_REG] &= ~0x1;
        _sp_reg[SP_STATUS_REG] &= ~0x303;

    }
    // Unknown task
    else
    {
        _sp_reg[SP_PC_REG] &= 0xFFF;
        Bus::plugins->rsp()->DoRspCycles(0xFFFFFFFF);
        _sp_reg[SP_PC_REG] |= save_pc;

        Bus::cpu->getcp0()->update_count();

        if (_mi_reg[MI_INTR_REG] & 0x1)
        {
            Bus::interrupt->add_interrupt_event(SP_INT, 0/*100*/);
        }

        _mi_reg[MI_INTR_REG] &= ~0x1;
        _sp_reg[SP_STATUS_REG] &= ~0x203;
    }
}

