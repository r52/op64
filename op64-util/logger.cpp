#include "optime.h"
#include "logger.h"

using namespace std;

static const char* log_levels[LOG_LEVEL_NUM] = {
    "[DEBUG]",
    "[INFO]",
    "[WARNING]",
    "[ERROR]"
};


void Logger::log(uint32_t level, const char* msg)
{
    // Allow the callback receiver to do whatever they
    // want with the message rather than follow the rules here
    if (_callback)
    {
        _callback(level, msg);
    }

    if (level < _minlevel)
        return;

    if (level > 3)
        level = 3;

    char buf[350];

    if (_useTimeStamp)
    {
        time_t now = time(NULL);
        char tmstr[100];
        tm t = op::localtime(now);
        strftime(tmstr, sizeof(tmstr), "%c", &t);
        _safe_sprintf(buf, 350, "%s: %s - %s\n", tmstr, log_levels[level], msg);
    }
    else
    {
        _safe_sprintf(buf, 350, "%s\n", msg);
    }

    if (_logToFile)
    {
        _logFile << buf;
    }
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

void Logger::logToFile(const char* msg)
{
    if (_logToFile)
    {
        _logFile << msg;
    }
}

