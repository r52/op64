#include "imemory.h"

#include <pif/pif.h>
#include <rom/sram.h>
#include <rom/flashram.h>


IMemory::~IMemory()
{
    uninitialize();
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

    fill_array(nops, 0, 2, 0);
}

void IMemory::uninitialize(void)
{
    using namespace Bus;

    if (nullptr != pif)
    {
        pif->uninitialize();
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

