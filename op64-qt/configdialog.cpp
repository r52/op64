#include <QtWidgets>

#include "configdialog.h"
#include "configpages.h"

#include <globalstrings.h>
#include <ui/configstore.h>

using namespace GlobalStrings;

ConfigDialog::ConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    createConfigWidgets();

    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 96));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMaximumWidth(128);
    contentsWidget->setMinimumWidth(128);
    contentsWidget->setMinimumHeight(300);
    contentsWidget->setSpacing(12);

    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(new EmulatorPage(_widgets));
    pagesWidget->addWidget(new PluginPage(_widgets));
    pagesWidget->setMinimumWidth(500);

    QPushButton *saveButton = new QPushButton(tr("Save"));
    QPushButton *closeButton = new QPushButton(tr("Close"));

    createIcons();
    contentsWidget->setCurrentRow(0);

    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveAndClose()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Config"));
}

void ConfigDialog::createIcons()
{
    QListWidgetItem *emuButton = new QListWidgetItem(contentsWidget);
    emuButton->setIcon(QIcon(":/Resources/processor.png"));
    emuButton->setText(tr("Emulator"));
    emuButton->setTextAlignment(Qt::AlignHCenter);
    emuButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *pluginButton = new QListWidgetItem(contentsWidget);
    pluginButton->setIcon(QIcon(":/Resources/gaming.png"));
    pluginButton->setText(tr("Plugins"));
    pluginButton->setTextAlignment(Qt::AlignHCenter);
    pluginButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    connect(contentsWidget,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
        this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

void ConfigDialog::createConfigWidgets()
{
    _widgets = new ConfigWidgets;
    _widgets->delaySICheckBox = new QCheckBox(tr("Delay SI"));
    _widgets->savePath = new QLineEdit;
    _widgets->gfxCombo = new QComboBox;
    _widgets->audioCombo = new QComboBox;
    _widgets->inputCombo = new QComboBox;
    _widgets->rspCombo = new QComboBox;

    ConfigStore& store = ConfigStore::getInstance();
    _widgets->delaySICheckBox->setChecked(store.getBool(CFG_SECTION_CORE, CFG_DELAY_SI));
    _widgets->savePath->setText(QString(store.getString(CFG_SECTION_CORE, CFG_SAVE_PATH).c_str()));
}

void ConfigDialog::saveAndClose(void)
{
    ConfigStore& store = ConfigStore::getInstance();

    store.set(CFG_SECTION_CORE, CFG_DELAY_SI, _widgets->delaySICheckBox->isChecked());
    // TODO save path

    QVariant data = _widgets->gfxCombo->currentData();

    if (data.isValid())
    {
        store.set(CFG_SECTION_CORE, CFG_GFX_PLUGIN, data.toString().toStdString());
    }

    data = _widgets->audioCombo->currentData();

    if (data.isValid())
    {
        store.set(CFG_SECTION_CORE, CFG_AUDIO_PLUGIN, data.toString().toStdString());
    }

    data = _widgets->inputCombo->currentData();

    if (data.isValid())
    {
        store.set(CFG_SECTION_CORE, CFG_INPUT_PLUGIN, data.toString().toStdString());
    }

    data = _widgets->rspCombo->currentData();

    if (data.isValid())
    {
        store.set(CFG_SECTION_CORE, CFG_RSP_PLUGIN, data.toString().toStdString());
    }

    store.saveConfig();

    close();
}

ConfigDialog::~ConfigDialog()
{
    // The actual widgets themselves should be cleaned up properly due to them being
    // added to the layouts of the config pages
    if (nullptr != _widgets)
    {
        delete _widgets; _widgets = nullptr;
    }
}
