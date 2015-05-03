#pragma once

#include <QTextEdit>

class LogWindow : public QTextEdit
{
    Q_OBJECT;

public:
    LogWindow();

public slots:
    void appendHtml(const QString& text);
};
