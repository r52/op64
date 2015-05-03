#include <oplog.h>

#include "logwindow.h"
#include "op_log_sink.h"


LogWindow::LogWindow()
{
    setWindowTitle(tr("op64 Log"));
    setMinimumSize(QSize(640, 480));
    setLineWrapMode(QTextEdit::NoWrap);
    setReadOnly(true);

    
    boost::shared_ptr< op_log_sink > backend(new op_log_sink());

    connect(backend.get(), SIGNAL(processLogRec(QString)), this, SLOT(appendHtml(QString)));

    boost::shared_ptr< op_log_sink_t > sink(new op_log_sink_t(backend));
    logging::core::get()->add_sink(sink);
}

void LogWindow::appendHtml(const QString& text)
{
    moveCursor(QTextCursor::End);
    insertHtml(text);
}
