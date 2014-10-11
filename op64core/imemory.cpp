#include "imemory.h"
#include "bus.h"
#include "pif.h"
#include "sram.h"
#include "flashram.h"


IMemory::IMemory()
{
    using namespace Bus;
    sp_dmem32 = _SP_DMEM;
    sp_imem32 = _SP_IMEM;
    sp_dmem8 = (uint8_t*)_SP_DMEM;
    sp_imem8 = (uint8_t*)_SP_IMEM;
    rdram = _rdram;
    rdram8 = (uint8_t*)_rdram;
    pif_ram32 = _PIF_RAM;
    pif_ram8 = (uint8_t*)_PIF_RAM;
    vi_reg = _vi_reg;
    mi_reg = _mi_reg;
    rdram_reg = _rdram_reg;
    sp_reg = _sp_reg;
    ai_reg = _ai_reg;
    pi_reg = _pi_reg;
    ri_reg = _ri_reg;
    si_reg = _si_reg;
    dp_reg = _dp_reg;
    dps_reg = _dps_reg;

    // Needs to be initialized by concrete memory
    pif = new PIF();
    sram = new SRAM();
    flashram = new FlashRam();
}

IMemory::~IMemory()
{
    using namespace Bus;
    sp_dmem32 = nullptr;
    sp_imem32 = nullptr;
    sp_dmem8 = nullptr;
    sp_imem8 = nullptr;
    rdram = nullptr;
    rdram8 = nullptr;
    pif_ram32 = nullptr;
    pif_ram8 = nullptr;
    vi_reg = nullptr;
    mi_reg = nullptr;
    rdram_reg = nullptr;
    sp_reg = nullptr;
    ai_reg = nullptr;
    pi_reg = nullptr;
    ri_reg = nullptr;
    si_reg = nullptr;
    dp_reg = nullptr;
    dps_reg = nullptr;

    if (nullptr != pif)
    {
        delete pif; pif = nullptr;
    }

    if (nullptr != sram)
    {
        delete sram; sram = nullptr;
    }

    if (nullptr != flashram)
    {
        delete flashram; flashram = nullptr;
    }
}

