#pragma once

#include <fstream>
#include <string>
#include "util.h"
#include <functional>

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NUM
};

typedef std::function<void(uint32_t, const char*)> LogCallback;

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
        log(0, msg);
    }

    inline void operator()(uint32_t level, const char* msg)
    {
        log(level, msg);
    }

    void log(uint32_t level, const char* msg);
    void logToFile(const char* msg);

    
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

    
private:
    bool _logToFile;
    bool _useTimeStamp;
    uint32_t _minlevel;
    LogCallback _callback;
    std::ofstream _logFile;
    std::string _filename = "op64.log";
};

#define LOG Logger::getInstance()

#define LOG_DEBUG(...) \
    { \
        char buf[250]; \
        _s_snprintf(buf, 250, __VA_ARGS__); \
        LOG(LOG_LEVEL_DEBUG, buf); \
    }

#define LOG_ERROR(...) \
    { \
        char buf[250]; \
        _s_snprintf(buf, 250, __VA_ARGS__); \
        LOG(LOG_LEVEL_ERROR, buf); \
    }

#define LOG_INFO(...) \
    { \
        char buf[250]; \
        _s_snprintf(buf, 250, __VA_ARGS__); \
        LOG(LOG_LEVEL_INFO, buf); \
    }

#define LOG_WARNING(...) \
    { \
        char buf[250]; \
        _s_snprintf(buf, 250, __VA_ARGS__); \
        LOG(LOG_LEVEL_WARNING, buf); \
    }

#define LOG_VERBOSE(...) \
    { \
        char buf[250]; \
        _s_snprintf(buf, 250, __VA_ARGS__); \
        LOG(LOG_LEVEL_VERBOSE, buf); \
    }

#ifdef _DEBUG
#define LOG_PC \
    char buf[20]; \
    _safe_sprintf(buf, 20, "0x%x\n", (uint32_t)_PC); \
    LOG.logToFile(buf);
#else
#define LOG_PC
#endif
