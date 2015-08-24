#include "imemory.h"

#include <oplog.h>

#include <pif/pif.h>
#include <rom/sram.h>
#include <rom/flashram.h>


IMemory::~IMemory()
{
    uninitialize(_bus);
}

bool IMemory::initialize(Bus* bus)
{
    if (nullptr == bus)
    {
        LOG_ERROR(IMemory) << "Invalid Bus";
        return false;
    }

    _bus = bus;
    bus->sram.reset(new SRAM);
    bus->flashram.reset(new FlashRam);
    bus->pif.reset(new PIF);

    return bus->pif->initialize();
}

void IMemory::uninitialize(Bus* bus)
{
    if (bus)
    {
        bus->pif.reset();
        bus->sram.reset();
        bus->flashram.reset();
        _bus = nullptr;
    }
}

