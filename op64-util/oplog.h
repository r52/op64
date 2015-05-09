#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include "oppreproc.h"

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(modname, "ModName", std::string)

#define _SCOPE_SPOT_ __func__ ":" __STR__(__LINE__)

#define LOG_LEVEL(name, level)  BOOST_LOG_TRIVIAL(level) << logging::add_value(modname, __STR__(name))
#define LOG_SCOPE_LEVEL(name, level) BOOST_LOG_NAMED_SCOPE(_SCOPE_SPOT_); LOG_LEVEL(name, level)
#define LOG_TRACE(name) LOG_SCOPE_LEVEL(name, trace)
#define LOG_DEBUG(name) LOG_SCOPE_LEVEL(name, debug)
#define LOG_INFO(name) LOG_SCOPE_LEVEL(name, info)
#define LOG_WARNING(name) LOG_SCOPE_LEVEL(name, warning)
#define LOG_ERROR(name) LOG_SCOPE_LEVEL(name, error)
#define LOG_FATAL(name) LOG_SCOPE_LEVEL(name, fatal)

void oplog_init(void);
