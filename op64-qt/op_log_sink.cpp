#include "op_log_sink.h"

#include <boost/log/expressions.hpp>
#include <QFileInfo>

//BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", attrs::named_scope::value_type)

static const char* logLevelFormatting[(uint32_t)logging::trivial::fatal + 1] = {
    "%1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;%2<br>",
    "<b>%1<font color='mediumblue'>%2</font></b><br>",
    "<b>%1<font color='forestgreen'>%2</font></b><br>",
    "<b>%1<font color='orange'>%2</font></b><br>",
    "<b>%1<font color='maroon'>%2</font></b><br>",
    "<b>%1<font color='maroon'>%2</font></b><br>"
};

static const char* module_string = "<font color='black'>%1:</font> ";

void op_log_sink::consume(logging::record_view const& rec, string_type const& line)
{
    QString msg = QString::fromStdString(rec[expr::smessage].get());
    uint32_t sev = (uint32_t) rec[logging::trivial::severity].get();
    QString module("");
    
    if (sev > 0)
    {
        module = QString(module_string).arg(QString::fromStdString(rec[modname].get()));
    }

    QString html = QString(logLevelFormatting[sev]).arg(module).arg(msg);

    emit processLogRec(html);
}
