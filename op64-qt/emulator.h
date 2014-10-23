/* state machine wrapper or something */
#pragma once
#include <cstdint>
#include <windows.h>
#include <QObject>
#include "qwindowdefs.h"

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

class Plugins;
class ICPU;
class IMemory;

class Emulator : public QObject
{
    Q_OBJECT

public:
    static Emulator& getInstance()
    {
        static Emulator instance;
        return instance;
    }

    ~Emulator();

    // execution
    bool loadRom(const char* filename);
    bool initializeHardware(Plugins* plugins, ICPU* cpu, IMemory* mem);
    bool execute(void);
    bool uninitializeHardware(void);
    bool isRomLoaded(void);
    void stopEmulator(void);

    inline EmuState getState(void)
    {
        return _state;
    }

    void setState(EmuState newstate);

public slots:
    void setLimitFPS(bool limit);
    void gameHardReset(void);
    void gameSoftReset(void);


signals:
    void stateChanged(EmuState newstate);

private:
    Emulator(void);

    Emulator(Emulator const&);
    void operator=(Emulator const&);

    void setupBus(Plugins* plugins, ICPU* cpu, IMemory* mem);

    EmuState _state;
};

#define EMU Emulator::getInstance()

