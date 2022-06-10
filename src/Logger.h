#pragma once

#include <utility>
#include <mutex>
#include <iostream>
#include <fmt/format.h>

#include "TerminalColor/TerminalColor.h"
#include "LoggerFormatter.h"

class Logger {
public:
    enum LogLevel {
        ERROR = 0,
        WARNING = 1,
        INFO = 2,
        VERBOSE = 3,
        DEBUG = 4
    };

    inline static LogLevel showLevel;

private:
    inline static std::mutex mutex;

    template <typename ...T>
    static void log(LogLevel logLevel, TerminalColor levelColor, const char *levelName, const char *format, T &&...args) {
        if (logLevel > showLevel) return;
        auto formatted = fmt::format(format, std::forward<T>(args)...);

        {
            std::lock_guard lock(mutex);
            std::cerr << TerminalColor::Reset << levelColor
                      << levelName << ": "
                      << TerminalColor::Reset
                      << formatted
                      << std::endl;
        }
    }

public:
    static void initialize(LogLevel showLevel) {
        Logger::showLevel = showLevel;
    }

#define LOG_FUNCTION(method, level, levelColor, levelName) \
    template <typename ...T> \
    inline static void (method)(const char *format, T &&...args) { \
        log((level), (levelColor), (levelName), format, std::forward<T>(args)...); \
    }

    LOG_FUNCTION(error, ERROR, TerminalColor::ForegroundRed, "Error")
    LOG_FUNCTION(warning, WARNING, TerminalColor::ForegroundYellow, "Warning")
    LOG_FUNCTION(info, INFO, TerminalColor::ForegroundBlue, "Info")
    LOG_FUNCTION(verbose, VERBOSE, TerminalColor::ForegroundMagenta, "Verbose")
    LOG_FUNCTION(debug, DEBUG, TerminalColor::ForegroundWhite, "Debug")

#undef LOG_FUNCTION
};
