#pragma once

#include <QDialog>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;

class QCheckBox;
class QLineEdit;
class QComboBox;

class PluginContainer;

struct ConfigWidgets
{
    QCheckBox* delaySICheckBox;
    QLineEdit* savePath;
    QComboBox* gfxCombo;
    QComboBox* audioCombo;
    QComboBox* inputCombo;
    QComboBox* rspCombo;
};

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget* parent = 0);
    ~ConfigDialog();

public slots:
    void saveAndClose(void);
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

private:
    void createIcons();
    void createConfigWidgets();

    QListWidget* contentsWidget;
    QStackedWidget* pagesWidget;
    ConfigWidgets* _widgets;
};
