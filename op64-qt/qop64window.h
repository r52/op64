#ifndef QOP64WINDOW_H
#define QOP64WINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_qop64window.h"
#include <QThread>

#include <cstdint>

class Plugins;
class ConfigDialog;
class Emulator;
class RenderWidget;
enum EmuState;

class LogWindow;
class QSettings;

class QOP64Window : public QMainWindow
{
    Q_OBJECT

public:
    QOP64Window(QWidget *parent = 0);
    ~QOP64Window();

protected:
    virtual void closeEvent(QCloseEvent* event);

private:
    void setupDirectories(void);
    void setupEmulationThread(void);
    void setupGUI(void);
    void connectGUIControls(void);

    void shudownEverything(void);

signals:
    void runEmulator(void);

public slots:
    void emulationFinished();
    void emulatorChangeState(EmuState newstate);
    void logCallback(uint32_t level, const char* msg);

private slots:
    void openRom(void);
    void openConfigDialog(void);
    void toggleShowLog(bool show);
    void showAboutDialog(void);

private:
    Ui::QOP64WindowClass ui;
    QString _romFile;
    QThread _emuThread;
    Emulator* _emu;
    ConfigDialog* cfgDialog;
    RenderWidget* renderWidget;
    LogWindow* _logWindow;
};

#endif // QOP64WINDOW_H
