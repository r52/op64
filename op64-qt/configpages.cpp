#include <QtWidgets>
#include <boost/filesystem.hpp>

#include "configpages.h"
#include "configdialog.h"
#include "configstore.h"

#include "library.h"
#include "plugins.h"
#include "logger.h"

#ifdef _MSC_VER
#define LIB_FILTER "*.dll"
#else
#define LIB_FILTER "*.so"
#endif


EmulatorPage::EmulatorPage(ConfigWidgets* widgets, QWidget *parent)
    : _widgets(widgets), QWidget(parent)
{
    QGroupBox *emulationGroup = new QGroupBox(tr("Emulation Options"));

    QGroupBox *pathsGroup = new QGroupBox(tr("Paths"));

    QLabel* saveLabel = new QLabel(tr("Save Path"));

    QVBoxLayout *emuLayout = new QVBoxLayout;
    emuLayout->addWidget(_widgets->delaySICheckBox);
    emulationGroup->setLayout(emuLayout);

    QVBoxLayout *pathsLayout = new QVBoxLayout;
    pathsLayout->addWidget(saveLabel);
    pathsLayout->addWidget(_widgets->savePath);
    pathsGroup->setLayout(pathsLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(emulationGroup);
    mainLayout->addWidget(pathsGroup);
    mainLayout->addSpacing(12);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

PluginPage::PluginPage(ConfigWidgets* widgets, QWidget *parent)
    : _widgets(widgets), QWidget(parent)
{
    QGroupBox *configGroup = new QGroupBox(tr("Plugins"));

    QLabel *gfxLabel = new QLabel(tr("Graphics Plugin:"));
    //QPushButton* gfxAbout = new QPushButton(tr("About"));
    //gfxAbout->setFixedWidth(50);
    populateGfxPlugins();

    QHBoxLayout *gfxLayout = new QHBoxLayout;
    gfxLayout->addWidget(gfxLabel);
    gfxLayout->addWidget(_widgets->gfxCombo);
    //gfxLayout->addWidget(gfxAbout);

    QLabel *audioLabel = new QLabel(tr("Audio Plugin:"));
    //QPushButton* audioAbout = new QPushButton(tr("About"));
    //audioAbout->setFixedWidth(50);
    populateAudioPlugins();

    QHBoxLayout *audioLayout = new QHBoxLayout;
    audioLayout->addWidget(audioLabel);
    audioLayout->addWidget(_widgets->audioCombo);
    //audioLayout->addWidget(audioAbout);

    QLabel *inputLabel = new QLabel(tr("Input Plugin:"));
    //QPushButton* inputAbout = new QPushButton(tr("About"));
    //inputAbout->setFixedWidth(50);
    populateInputPlugins();

    QHBoxLayout *inputLayout = new QHBoxLayout;
    inputLayout->addWidget(inputLabel);
    inputLayout->addWidget(_widgets->inputCombo);
    //inputLayout->addWidget(inputAbout);

    QLabel *rspLabel = new QLabel(tr("RSP Plugin:"));
    //QPushButton* rspAbout = new QPushButton(tr("About"));
    //rspAbout->setFixedWidth(50);
    populateRSPPlugins();

    QHBoxLayout *rspLayout = new QHBoxLayout;
    rspLayout->addWidget(rspLabel);
    rspLayout->addWidget(_widgets->rspCombo);
    //rspLayout->addWidget(rspAbout);

    QVBoxLayout *configLayout = new QVBoxLayout;
    configLayout->addLayout(gfxLayout);
    configLayout->addLayout(audioLayout);
    configLayout->addLayout(inputLayout);
    configLayout->addLayout(rspLayout);
    configGroup->setLayout(configLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

void PluginPage::populateGfxPlugins(void)
{
    populatePluginType(PLUGIN_TYPE_GFX);
}

void PluginPage::populateAudioPlugins(void)
{
    populatePluginType(PLUGIN_TYPE_AUDIO);
}

void PluginPage::populateInputPlugins(void)
{
    populatePluginType(PLUGIN_TYPE_CONTROLLER);
}

void PluginPage::populateRSPPlugins(void)
{
    populatePluginType(PLUGIN_TYPE_RSP);
}

void PluginPage::populatePluginType(uint16_t type)
{
    std::string cfgPath;
    std::string curPlug;
    QComboBox* combo = nullptr;

    switch (type)
    {
    case PLUGIN_TYPE_GFX:
        cfgPath = CFG_GFX_PATH;
        curPlug = CFG_GFX_PLUGIN;
        combo = _widgets->gfxCombo;
        break;
    case PLUGIN_TYPE_AUDIO:
        cfgPath = CFG_AUDIO_PATH;
        curPlug = CFG_AUDIO_PLUGIN;
        combo = _widgets->audioCombo;
        break;
    case PLUGIN_TYPE_CONTROLLER:
        cfgPath = CFG_INPUT_PATH;
        curPlug = CFG_INPUT_PLUGIN;
        combo = _widgets->inputCombo;
        break;
    case PLUGIN_TYPE_RSP:
        cfgPath = CFG_RSP_PATH;
        curPlug = CFG_RSP_PLUGIN;
        combo = _widgets->rspCombo;
        break;
    default:
        LOG_ERROR("GUI: bad plugin type");
        return;
        break;
    }


    QDir dir(QString(ConfigStore::getInstance().getString(CFG_SECTION_CORE, cfgPath).c_str()), LIB_FILTER);
    QFileInfoList list = dir.entryInfoList();

    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo plugfile = list.at(i);

        LibHandle plugHandle;
        std::string plugPath = plugfile.filePath().toStdString();
        if (!opLoadLib(&plugHandle, plugPath.c_str()))
        {
            continue;
        }

        void(*GetDllInfo) (PLUGIN_INFO * PluginInfo);
        GetDllInfo = (void(*)(PLUGIN_INFO *))opLibGetFunc(plugHandle, "GetDllInfo");

        if (GetDllInfo == nullptr)
        {
            opLibClose(plugHandle);
            continue;
        }

        PLUGIN_INFO info;
        GetDllInfo(&info);

        if (info.Type != type)
        {
            opLibClose(plugHandle);
            continue;
        }

        combo->addItem(QString(info.Name), plugPath.c_str());
        opLibClose(plugHandle);
    }

    std::string curFile = ConfigStore::getInstance().getString(CFG_SECTION_CORE, curPlug);
    int index = combo->findData(curFile.c_str());
    if (index != -1)
    {
        combo->setCurrentIndex(index);
    }
}
