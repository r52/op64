#pragma once

#include <fstream>
#include <string>
#include "util.h"
#include <functional>

enum LogLevel {
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NUM
};

typedef std::function<void(const char*)> LogCallback;

class Logger
{
public:
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    inline void operator()(const char* msg)
    {
        log(msg, 0);
    }

    inline void operator()(const char* msg, uint32_t level)
    {
        log(msg, level);
    }

    void log(const char* msg, uint32_t level);

    
    /* setLogToFile 
    bool    toFile  set whether to log to file or not
    return  bool    whether the change was successful
    */
    bool setLogToFile(bool toFile);
    inline bool isLoggingToFile(void)
    {
        return _logToFile;
    }

    void setLogCallback(LogCallback callback);

    inline bool getUseTimeStamp(void)
    {
        return _useTimeStamp;
    }

    inline void setUseTimeStamp(bool timestamp)
    {
        _useTimeStamp = timestamp;
    }

    inline void setMinLogLevel(uint32_t level)
    {
        if (level > 3)
            level = 3;

        _minlevel = level;
    }

private:
    Logger(void);
    ~Logger(void);

    Logger(Logger const&);
    void operator=(Logger const&);

    void log(const char* msg);
    
private:
    bool _logToFile;
    bool _useTimeStamp;
    uint32_t _minlevel;
    LogCallback _callback;
    std::ofstream _logFile;
    std::string _filename = "log.log";
};

#define LOG Logger::getInstance()

#define LOG_DEBUG(...) \
    char buf[250]; \
    _safe_sprintf(buf, 250, __VA_ARGS__); \
    LOG(buf, LOG_LEVEL_DEBUG);

#define LOG_ERROR(...) \
    char buf[250]; \
    _safe_sprintf(buf, 250, __VA_ARGS__); \
    LOG(buf, LOG_LEVEL_ERROR);

#define LOG_INFO(...) \
    char buf[250]; \
    _safe_sprintf(buf, 250, __VA_ARGS__); \
    LOG(buf, LOG_LEVEL_INFO);

#define LOG_WARNING(...) \
    char buf[250]; \
    _safe_sprintf(buf, 250, __VA_ARGS__); \
    LOG(buf, LOG_LEVEL_WARNING);

#define LOG_WARNING(...) \
    char buf[250]; \
    _safe_sprintf(buf, 250, __VA_ARGS__); \
    LOG(buf, LOG_LEVEL_WARNING);

#ifdef _DEBUG
#define LOG_PC \
    if (LOG.isLoggingToFile()) \
    { \
        char buf[20]; \
        _safe_sprintf(buf, 20, "0x%x", (uint32_t)_PC); \
        LOG(buf, LOG_LEVEL_INFO); \
    }
#else
#define LOG_PC
#endif

