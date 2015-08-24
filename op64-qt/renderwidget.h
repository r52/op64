#pragma once

#include <cstdint>
#include <QWidget>

class Emulator;

class RenderWidget : public QWidget
{
    Q_OBJECT;

public:
    RenderWidget(QString title, Emulator* emu, QWidget* parent = 0);
    ~RenderWidget();

    virtual QPaintEngine* paintEngine() const { return 0; }

public slots:
    void toggleFullscreen(void);

protected:
    virtual void closeEvent(QCloseEvent * event);

private slots:
    void displayVI();

private:
    Emulator* _emu;
    QString romName;
};
