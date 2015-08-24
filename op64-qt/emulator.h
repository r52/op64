/* state machine wrapper or something */
#pragma once
#include <cstdint>
#include <memory>

#include <QObject>
#include "qwindowdefs.h"

#include <core/bus.h>
#include <mem/imemory.h>
#include <cpu/icpu.h>
#include <plugin/plugincontainer.h>

enum EmuState : uint8_t
{
    DEAD = 0,
    ROM_LOADED,
    HARDWARE_INITIALIZED,
    EMU_RUNNING,
    EMU_PAUSED,
    EMU_STOPPED
};

Q_DECLARE_METATYPE(EmuState);


class Emulator : public QObject
{
    Q_OBJECT

public:
    Emulator(WId mainwindow);
    ~Emulator() = default;

    // execution
    bool loadRom(const char* filename, QString& romname);
    bool initializeHardware(PluginContainer* plugins, ICPU* cpu, IMemory* mem);
    bool execute(void);
    bool isRomLoaded(void);
    void stopEmulator(void);
    void toggleFullScreen(void);

    inline EmuState getState(void)
    {
        return _state;
    }

    void setState(EmuState newstate);

    inline void setRenderWindow(WId renderwindow)
    {
        _renderwindow = renderwindow;
    }

public slots:
    void setLimitFPS(bool limit);
    void gameHardReset(void);
    void gameSoftReset(void);
    void runEmulator(void);

    // plugin config
    void showGraphicsConfig(void);
    void showAudioConfig(void);
    void showInputConfig(void);
    void showRSPConfig(void);

signals:
    void stateChanged(EmuState newstate);
    void emulatorFinished(void);

private:
    // Dont implement
    Emulator(void);
    Emulator(Emulator const&);
    void operator=(Emulator const&);

    EmuState _state;

    // Windows
    WId _mainwindow;
    WId _renderwindow;

    // devices
    std::unique_ptr<PluginContainer> _plugins;
    std::unique_ptr<ICPU> _cpu;
    std::unique_ptr<IMemory> _mem;
    std::unique_ptr<Bus> _bus;
};

