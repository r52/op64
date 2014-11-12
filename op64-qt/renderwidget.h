#pragma once

#include <cstdint>
#include <QWidget>

class Emulator;

class RenderWidget : public QWidget
{
    Q_OBJECT;

public:
    RenderWidget(Emulator* emu, QWidget* parent = 0);
    ~RenderWidget();

    virtual QPaintEngine* paintEngine() const { return 0; }

protected:
    virtual void closeEvent(QCloseEvent * event);

private:
    void displayVI(uint64_t framerate);

private:
    Emulator* _emu;
    QString romName;
};
