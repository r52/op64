#include "logwindow.h"

void LogWindow::appendHtml(const QString& text)
{
    moveCursor(QTextCursor::End);
    insertHtml(text);
}
