#pragma once

#include <vector>
#include <string>

#include "Logger.h"

struct Arguments {
    std::vector<std::string> interfaces;
    Logger::LogLevel logLevel;
    size_t alarmInterval;
    size_t routeProbeInterval;
    size_t routeProbeRetries;
    bool preserveRoutesOnExit;
};

Arguments parseArguments(int argc, char *argv[]);
