#include "bus.h"
#include "icpu.h"
#include "imemory.h"
#include "interrupthandler.h"
#include "rom.h"
#include "pif.h"
#include "logger.h"
#include "plugins.h"
#include "systiming.h"

namespace Bus
{
    // unmanaged devices
    Rom* rom = nullptr;
    ICPU* cpu = nullptr;
    IMemory* mem = nullptr;
    Plugins* plugins = nullptr;
    SysTiming* systimer = nullptr;

    // managed devices
    InterruptHandler* interrupt = nullptr;
    PIF* pif = nullptr;
    CONTROL* controllers = nullptr;
    SRAM* sram = nullptr;
    FlashRam* flashram = nullptr;

    // regs
    Register64* reg = nullptr;
    Register64* hi = nullptr;
    Register64* lo = nullptr;
    bool* llbit = nullptr;
    uint32_t* cp0_reg = nullptr;
    uint32_t* FCR0 = nullptr;
    uint32_t* FCR31 = nullptr;
    float** s_reg = nullptr;
    double** d_reg = nullptr;
    uint64_t* fgr = nullptr;

    // cpu state
    ProgramCounter* PC = nullptr;
    uint32_t* last_instr_addr = nullptr;
    uint32_t* next_interrupt = nullptr;
    Instruction* cur_instr = nullptr;
    uint32_t* skip_jump = nullptr;
    bool* delay_slot = nullptr;
    std::atomic<bool> stop = true;

    // mem
    uint8_t* rom_image = nullptr;
    uint32_t* sp_dmem32 = nullptr;
    uint32_t* sp_imem32 = nullptr;
    uint8_t* sp_dmem8 = nullptr;
    uint8_t* sp_imem8 = nullptr;
    uint32_t* rdram = nullptr;
    uint8_t* rdram8 = nullptr;
    uint32_t* pif_ram32 = nullptr;
    uint8_t* pif_ram8 = nullptr;


    // in-memory regs
    uint32_t* vi_reg = nullptr;
    uint32_t* mi_reg = nullptr;
    uint32_t* rdram_reg = nullptr;
    uint32_t* sp_reg = nullptr;
    uint32_t* ai_reg = nullptr;
    uint32_t* pi_reg = nullptr;
    uint32_t* ri_reg = nullptr;
    uint32_t* si_reg = nullptr;
    uint32_t* dp_reg = nullptr;
    uint32_t* dps_reg = nullptr;

    // vi state
    uint32_t* next_vi = nullptr;
    int32_t* vi_field = nullptr;

    // interrupt state
    bool* SPECIAL_done = nullptr;
    bool* perform_hard_reset = nullptr;
    bool* interrupt_unsafe_state = nullptr;

    // core control
    std::atomic<bool> limitVI = true;

    // local bus state check. if true, then machine ready to execute
    static bool devicesInitialized = false;

    bool connectRom(Rom* dev)
    {
        if (nullptr != rom)
        {
            LOG_WARNING("Bus: old rom not properly destroyed");
            delete rom; rom = nullptr;
        }

        rom = dev;
        return true;
    }

    bool connectMemory(IMemory* dev)
    {
        if (nullptr != mem)
        {
            LOG_WARNING("Bus: old memory not properly destroyed");
            delete mem; mem = nullptr;
        }

        mem = dev;
        return true;
    }

    bool connectCPU(ICPU* dev)
    {
        if (nullptr != cpu)
        {
            LOG_WARNING("Bus: old cpu not properly destroyed");
            delete cpu; cpu = nullptr;
        }

        cpu = dev;
        return true;
    }

    bool connectPlugins(Plugins* dev)
    {
        if (nullptr != plugins)
        {
            LOG_WARNING("Bus: old plugins not properly cleared");
        }

        plugins = dev;
        return true;
    }

    bool initializeDevices(void)
    {
        if (nullptr == rom)
        {
            LOG_ERROR("Bus: no rom connected");
            return false;
        }

        if (nullptr == mem)
        {
            LOG_ERROR("Bus: no memory connected");
            return false;
        }

        if (nullptr == cpu)
        {
            LOG_ERROR("Bus: no cpu connected");
            return false;
        }

        if (nullptr == plugins)
        {
            LOG_ERROR("Bus: no plugins connected");
            return false;
        }

        mem->initialize();
        cpu->initialize();

        if (!plugins->initialize())
        {
            LOG_ERROR("Bus: one or more plugins failed to initialize");
            stop = true;       // unnecessary, but just in case
            return false;
        }

        devicesInitialized = true;

        return true;
    }

    void executeMachine(void)
    {
        if (!devicesInitialized)
        {
            LOG_ERROR("Bus: execute failed. devices not initialized");
            return;
        }

        plugins->RomOpened();

        systimer = new SysTiming(rom->getViLimit());
        systimer->startTimers();

        cpu->execute();

        plugins->RomClosed();

        delete systimer; systimer = nullptr;
    }

    bool disconnectDevices(void)
    {
        if (!stop)
        {
            LOG_ERROR("Bus: cannot destroy devices while machine is running!");
            return false;
        }

        if (nullptr != mem)
        {
            //delete mem;
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

}
