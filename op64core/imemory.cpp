#include "imemory.h"
#include "pif.h"
#include "sram.h"
#include "flashram.h"


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

