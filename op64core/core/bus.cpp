#include <oplog.h>

#include "bus.h"

#include <cpu/icpu.h>
#include <cpu/interrupthandler.h>
#include <mem/imemory.h>
#include <rom/rom.h>
#include <pif/pif.h>
#include <plugin/plugincontainer.h>
#include <core/systiming.h>
#include <cheat/cheatengine.h>
#include <rcp/rcp.h>

// Exposed states for device communication
namespace Bus
{
    // unmanaged devices
    ICPU* cpu = nullptr;
    IMemory* mem = nullptr;
    PluginContainer* plugins = nullptr;

    // managed devices
    SysTiming* systimer = nullptr;
    CheatEngine* cheat = nullptr;
    Rom* rom = nullptr;
    InterruptHandler* interrupt = nullptr;
    PIF* pif = nullptr;
    SRAM* sram = nullptr;
    FlashRam* flashram = nullptr;

    // controllers (persistent)
    RCP* rcp = nullptr;
    RDRAMController* rdram = nullptr;

    // regs
    uint32_t* cp0_reg = nullptr;

    // controller data
    CONTROL controllers[4];

    // cpu state
    ProgramCounter* PC = nullptr;
    uint32_t last_jump_addr = 0;
    uint32_t next_interrupt = 0;
    uint32_t skip_jump = 0;
    std::atomic<bool> stop{true};

    // vi state
    uint32_t next_vi = 0;
    uint32_t vi_delay = 0;
    int32_t vi_field = 0;

    // interrupt state
    bool interrupt_unsafe_state = false;

    // core control
    std::atomic<bool> doHardReset{false};
    std::atomic<bool> limitVI{true};

    // local bus state check. if true, then machine ready to execute
    static bool devicesInitialized = false;

    bool connectRom(Rom* dev)
    {
        if (nullptr != rom)
        {
            LOG_WARNING(Bus) << "Old rom not properly destroyed";
            delete rom; rom = nullptr;
        }

        rom = dev;
        return true;
    }

    bool connectMemory(IMemory* dev)
    {
        if (nullptr != mem)
        {
            LOG_WARNING(Bus) << "Old memory not properly destroyed";
            delete mem; mem = nullptr;
        }

        mem = dev;
        return true;
    }

    bool connectCPU(ICPU* dev)
    {
        if (nullptr != cpu)
        {
            LOG_WARNING(Bus) << "Old cpu not properly destroyed";
            delete cpu; cpu = nullptr;
        }

        cpu = dev;
        return true;
    }

    bool connectPlugins(PluginContainer* dev)
    {
        if (nullptr != plugins)
        {
            LOG_WARNING(Bus) << "Old plugins not properly cleared";
        }

        plugins = dev;
        return true;
    }

    bool initializeDevices(void)
    {
        LOG_INFO(Bus) << "Initializing devices";

        if (nullptr == rom)
        {
            LOG_ERROR(Bus) << "No ROM connected";
            return false;
        }

        if (nullptr == mem)
        {
            LOG_ERROR(Bus) << "No memory connected";
            return false;
        }

        if (nullptr == cpu)
        {
            LOG_ERROR(Bus) << "No cpu connected";
            return false;
        }

        if (nullptr == plugins)
        {
            LOG_ERROR(Bus) << "No plugins connected";
            return false;
        }

        mem->initialize();
        cpu->initialize();

        if (!plugins->initialize())
        {
            LOG_ERROR(Bus) << "One or more plugins failed to initialize";
            stop = true;       // unnecessary, but just in case
            return false;
        }

        devicesInitialized = true;
        LOG_INFO(Bus) << "Devices successfully initialized";

        return true;
    }

    void executeMachine(void)
    {
        if (!devicesInitialized)
        {
            LOG_ERROR(Bus) << "Execute failed. devices not initialized";
            return;
        }

        LOG_INFO(Bus) << "Executing emulator...";

        plugins->RomOpened();

        systimer = new SysTiming(rom->getViLimit());
        systimer->startTimers();

        cheat = new CheatEngine;
        cheat->addRomHacks(rom->getRomHacks());

        cpu->execute();

        LOG_INFO(Bus) << "Stopping emulator...";

        plugins->RomClosed();

        delete systimer; systimer = nullptr;
        delete cheat; cheat = nullptr;
    }

    bool disconnectDevices(void)
    {
        if (!stop)
        {
            LOG_ERROR(Bus) << "Cannot destroy devices while emulator is running!";
            return false;
        }

        LOG_INFO(Bus) << "Uninitializing devices";

        if (nullptr != mem)
        {
            mem->uninitialize();
            mem = nullptr;
        }

        if (nullptr != cpu)
        {
            //delete cpu;
            cpu = nullptr;
        }

        if (nullptr != rom)
        {
            delete rom;
            rom = nullptr;
        }

        if (nullptr != plugins)
        {
            plugins->StopEmulation();
            // just release it. owned by executing module (gui etc)
            plugins = nullptr;
        }

        devicesInitialized = false;

        return true;
    }

    void doSoftReset(void)
    {
        if (!devicesInitialized)
        {
            LOG_ERROR(Bus) << "Reset error. Devices not initialized";
            return;
        }

        LOG_INFO(Bus) << "Soft resetting emulator...";

        interrupt->softReset();
    }

    OPStatus BusStartup(void)
    {
        // Start up any persistent devices
        if (nullptr != rcp)
        {
            delete rcp; rcp = nullptr;
        }

        if (nullptr != rdram)
        {
            delete rdram; rdram = nullptr;
        }

        if (nullptr != pif)
        {
            delete pif; pif = nullptr;
        }

        rcp = new RCP;
        rdram = new RDRAMController;
        pif = new PIF;

        return OP_OK;
    }

    OPStatus BusShutdown(void)
    {
        // Shutdown any persistent devices
        if (nullptr != rcp)
        {
            delete rcp; rcp = nullptr;
        }

        if (nullptr != rdram)
        {
            delete rdram; rdram = nullptr;
        }

        if (nullptr != pif)
        {
            delete pif; pif = nullptr;
        }

        return OP_OK;
    }

}
