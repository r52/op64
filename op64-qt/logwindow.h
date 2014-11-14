#pragma once

#include <QTextEdit>

class LogWindow : public QTextEdit
{
    Q_OBJECT;

public slots:
    void appendHtml(const QString& text);
};
