#pragma once

#include <oplog.h>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>

#include <QObject>

class op_log_sink : public QObject, public sinks::basic_formatted_sink_backend<char, sinks::synchronized_feeding>
{
    Q_OBJECT;

public:
    void consume(logging::record_view const& rec, string_type const& line);

signals:
    void processLogRec(QString rec);
};

typedef sinks::synchronous_sink< op_log_sink > op_log_sink_t;
