#pragma once

#include <QWidget>
#include <cstdint>

struct ConfigWidgets;

class EmulatorPage : public QWidget
{
public:
    EmulatorPage(ConfigWidgets* widgets, QWidget *parent = 0);

private:
    ConfigWidgets* _widgets;
};

class PluginPage: public QWidget
{
public:
    PluginPage(ConfigWidgets* widgets, QWidget *parent = 0);

private:
    void populateGfxPlugins(void);
    void populateAudioPlugins(void);
    void populateInputPlugins(void);
    void populateRSPPlugins(void);

    void populatePluginType(uint16_t type);

private:
    ConfigWidgets* _widgets;
};
