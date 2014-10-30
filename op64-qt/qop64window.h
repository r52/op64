#ifndef QOP64WINDOW_H
#define QOP64WINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_qop64window.h"
#include <QThread>


class Plugins;
class ConfigDialog;
class Emulator;
class RenderWidget;
enum EmuState;

class QOP64Window : public QMainWindow
{
    Q_OBJECT

public:
    QOP64Window(QWidget *parent = 0);
    ~QOP64Window();

private:
    void setupDirectories(void);
    void setupEmulationThread(void);
    void setupGUI(void);
    void connectGUIControls(void);
    void setupGLFW(void);

    void logCallback(const char* msg);

signals:
    void runEmulator(void);

public slots:
    void emulationFinished();
    void emulatorChangeState(EmuState newstate);

private slots:
    void openRom(void);
    void showGraphicsConfig(void);
    void showAudioConfig(void);
    void showInputConfig(void);
    void showRSPConfig(void);
    void openConfigDialog(void);
    void toggleShowLog(bool show);
    void showAboutDialog(void);

private:
    Ui::QOP64WindowClass ui;
    QString _romFile;
    QThread _emuThread;
    Emulator* _emu;
    Plugins* _plugins;
    ConfigDialog* cfgDialog;
    RenderWidget* renderWidget;
};

#endif // QOP64WINDOW_H
