#include "Arguments.h"

#include <regex>

#include "ArgumentParser/ArgumentParser.h"

Arguments parseArguments(int argc, char *argv[]) {
    Arguments arguments;
    ArgumentParser(argc, argv)
        .setProgramDescription(
            "Magpie " BUILD_VERSION " (https://github.com/Menci/magpie)\n"
            "               .-'-._  \n"
            "              /    e<  \n"
            "          _.-''';  (   \n"
            "_______.-''-._.-'  /   \n"
            "====---:_''-'     /    \n"
            "         '-=. .-'`     \n"
            "           _|_\\_      \n"
            "Bidirectional NDP proxy and route maintainer to relay an IPv6 SLAAC network."
        )
        .addOption(
            "interfaces", "i",
            "list",
            "List of interface names to proxy NDP among (separated with ',').",
            [&] (const std::string &s) {
                std::regex re(",");
                arguments.interfaces = std::vector<std::string>(
                    std::sregex_token_iterator(s.begin(), s.end(), re, -1),
                    std::sregex_token_iterator()
                );
                return std::nullopt;
            },
            false
        )
        .addOption(
            "log-level", "l",
            "level",
            "The log level of output. Possible values are: error, warning, info, verbose, debug.",
            [&] (std::string s) -> std::optional<std::string> {
                for (auto &ch : s) ch = std::tolower(ch);
                if (s == "error") arguments.logLevel = Logger::ERROR;
                else if (s == "warning") arguments.logLevel = Logger::WARNING;
                else if (s == "info") arguments.logLevel = Logger::INFO;
                else if (s == "verbose") arguments.logLevel = Logger::VERBOSE;
                else if (s == "debug") arguments.logLevel = Logger::DEBUG;
                else return "unknown log level: " + s;
                return std::nullopt;
            },
            true,
            "info"
        )
        .addOption(
            "alarm-interval", "a",
            "seconds",
            "The interval of alarm to check and reprobe routes.",
            ArgumentParser::integerParser(arguments.alarmInterval),
            true, "10"
        )
        .addOption(
            "probe-interval", "p",
            "seconds",
            "The interval of timeout to reprobe a route.",
            ArgumentParser::integerParser(arguments.routeProbeInterval),
            true, "60"
        )
        .addOption(
            "probe-retries", "r",
            "count",
            "The max retries of reprobing of a route before deleting as expired.",
            ArgumentParser::integerParser(arguments.routeProbeRetries),
            true, "5"
        )
        .addOption(
            "routes-save-file", "f",
            "path",
            "The file to save current routes info between restarts.",
            ArgumentParser::stringParser(arguments.routesSaveFile),
            true, ""
        )
        .parse();
    return arguments;

}
