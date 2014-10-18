#pragma once

#include <cstdint>
#include <QWidget>

class RenderWidget : public QWidget
{
    Q_OBJECT;

public:
    RenderWidget(QWidget* parent = 0);

    virtual QPaintEngine* paintEngine() const { return 0; }

protected:
    virtual void closeEvent(QCloseEvent * event);

private:
    void displayVI(uint64_t framerate);
};
