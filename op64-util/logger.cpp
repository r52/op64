#include <ctime>
#include "logger.h"

using namespace std;

static const char* log_levels[LOG_LEVEL_NUM] = {
    "[INFO]",
    "[DEBUG]",
    "[WARNING]",
    "[ERROR]"
};

void Logger::log(const char* msg)
{
    if (_callback)
    {
        _callback(msg);
    }

    if (_logToFile && _logFile.is_open())
    {
        _logFile << msg;
    }
}

void Logger::log(const char* msg, uint32_t level)
{
    if (level < _minlevel)
        return;

    if (level > 3)
        level = 3;

    char buf[350];

    if (_useTimeStamp)
    {
        time_t now;
        time(&now);
        char* timstr = ctime(&now);
        timstr[strlen(timstr) - 1] = '\0';

        _safe_sprintf(buf, 350, "%s: %s - %s\n", timstr, log_levels[level], msg);
    }
    else
    {
        _safe_sprintf(buf, 350, "%s\n", msg);
    }

    log(buf);
}

bool Logger::setLogToFile(bool toFile)
{
    if (toFile)
    {
        if (_logToFile)
        {
            // already logging to file
            return true;
        }

        if (_logFile.is_open())
        {
            _logFile.close();
        }

        _logFile.open(_filename, ios::out | ios::ate);
        if (_logFile.is_open() && _logFile.good())
        {
            _logToFile = true;
            return true;
        }
        
        // Fail
        _logFile.close();        
    }
    else
    {
        if (!_logToFile)
        {
            // already not logging to file
            return true;
        }

        _logToFile = false;
        
        if (_logFile.is_open())
        {
            _logFile.close();
        }

        return true;
    }


    return false;
}

void Logger::setLogCallback(LogCallback callback)
{
    _callback = callback;
}

Logger::~Logger(void)
{
    _callback = nullptr;

    if (_logFile.is_open())
    {
        _logFile.close();
    }
}

Logger::Logger(void) :
_logToFile(false),
_callback(nullptr),
_useTimeStamp(true),
_minlevel(3)
{
    //setLogToFile(true);
}

