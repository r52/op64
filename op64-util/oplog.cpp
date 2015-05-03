#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/make_shared.hpp>

#include "oplog.h"


void oplog_init()
{
    boost::shared_ptr< logging::core > core = logging::core::get();
    core->add_global_attribute("TimeStamp", attrs::local_clock());
    core->add_global_attribute("Scope", attrs::named_scope());
    core->add_global_attribute("ThreadID", attrs::current_thread_id());

    // TODO: change this to an option
    bool logToFile = false;

    if (logToFile)
    {
        typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink_t;

        boost::shared_ptr< file_sink_t > sink(new file_sink_t(
            keywords::file_name = "op64_%N.log",
            keywords::rotation_size = 5 * 1024 * 1024
            ));

        sink->set_formatter(
            expr::format("[%1%]    %2%    %3%    %4%    %5%")
            % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
            % logging::trivial::severity
            % expr::attr< unsigned int >("ThreadID")
            % expr::format_named_scope("Scope", keywords::format = "%f:%l    %n")
            % expr::smessage
            );

        sink->set_filter(logging::trivial::severity >= logging::trivial::warning);

        core->add_sink(sink);
    }
}
