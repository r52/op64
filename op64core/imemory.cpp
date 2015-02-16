#include "imemory.h"
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
    sp2_reg = _sp2_reg;

    pi_reg = _pi_reg;
    ri_reg = _ri_reg;
    si_reg = _si_reg;
    dp_reg = _dp_reg;
    dps_reg = _dps_reg;

    // The pif needs to be persistent in pj64 spec
    pif = new PIF();
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
    sp2_reg = nullptr;

    pi_reg = nullptr;
    ri_reg = nullptr;
    si_reg = nullptr;
    dp_reg = nullptr;
    dps_reg = nullptr;

    uninitialize();

    if (nullptr != pif)
    {
        delete pif; pif = nullptr;
    }
}

void IMemory::initialize(void)
{
    using namespace Bus;

    // Sanity checks

    if (nullptr != sram)
    {
        delete sram; sram = nullptr;
    }

    if (nullptr != flashram)
    {
        delete flashram; flashram = nullptr;
    }

    sram = new SRAM();
    flashram = new FlashRam();

    pif->initialize();
}

void IMemory::uninitialize(void)
{
    using namespace Bus;

    pif->uninitialize();

    if (nullptr != sram)
    {
        delete sram; sram = nullptr;
    }

    if (nullptr != flashram)
    {
        delete flashram; flashram = nullptr;
    }
}

