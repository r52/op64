#pragma once
#include <QObject>

class Plugins;
class ICPU;
class IMemory;

class EmulatorThread : public QObject
{
    Q_OBJECT

public:
    EmulatorThread(Plugins* plugins);
    ~EmulatorThread();

public slots:
    void runEmulator(void);

signals:
    void emulatorFinished(void);

private:
    Plugins* _plugins;
    ICPU* _cpu;
    IMemory* _mem;
};

