/* state machine wrapper or something */
#pragma once
#include <cstdint>
#include <windows.h>

enum EmuState
{
    DEAD = 0,
    ROM_LOADED,
    HARDWARE_INITIALIZED,
    EMU_RUNNING,
    EMU_PAUSED,
    EMU_STOPPED,
    HARDWARE_DEINITIALIZED      // dont think this is needed
};

class Emulator
{

public:
    static Emulator& getInstance()
    {
        static Emulator instance;
        return instance;
    }

    ~Emulator();


    bool loadRom(const char* filename);
    bool initializeHardware(void);
    bool execute(void);
    bool uninitializeHardware(void);
    bool isRomLoaded(void);
    void stopEmulator(void);

    inline void registerRenderWindow(HWND win)
    {
        _renderWindow = win;
    }

    inline EmuState getState(void)
    {
        return _state;
    }

private:
    Emulator(void) :
        _state(DEAD),
        _renderWindow(nullptr)
    {}

    Emulator(Emulator const&);
    void operator=(Emulator const&);
    void cleanup(void);
    void cleanupDevices(void);

    EmuState _state;
    HWND _renderWindow;
};

#define EMU Emulator::getInstance()

