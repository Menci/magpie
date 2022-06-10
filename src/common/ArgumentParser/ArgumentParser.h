#ifndef _MENCI_ARGUMENTPARSER_H
#define _MENCI_ARGUMENTPARSER_H

#include <cassert>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <optional>

class ArgumentParser {
public:
    using ParserFunction = std::function<std::optional<std::string> (const std::string &)>;

    template <typename T>
    using ValidatorFunction = std::function<std::optional<std::string> (const T &)>;

private:
    struct Argument {
        std::string name, alias, valueName;
        std::string description;
        bool optional, specfied;
        std::string defaultValue;
        ParserFunction parserFunction;
    };

    size_t argc;
    char **argv;

    std::map<std::string, std::shared_ptr<Argument>> mapNameArgument;
    std::map<std::string, std::shared_ptr<Argument>> mapAliasArgument;
    std::vector<std::shared_ptr<Argument>> options;
    std::vector<Argument> positionalArguments;

    std::string programDescription;

public:
    ArgumentParser(int argc, char **argv) : argc((size_t)argc), argv(argv) {}

    template <typename IntType>
    static ParserFunction integerParser(IntType &var,
                                        ValidatorFunction<IntType> validator = nullptr) {
        return [=, &var](const std::string &str) -> std::optional<std::string> {
            try {
                IntType val;
                std::stringstream(str) >> val;
                if (validator) {
                    std::optional<std::string> errorMessage = validator(val);
                    if (errorMessage) return errorMessage;
                }

                var = val;
                return std::nullopt;
            } catch (const std::exception &e) {
                return "Not a valid integer: " + str;
            }
        };
    }

    template <typename StringType = std::string>
    static ParserFunction stringParser(StringType &var,
                                       std::function<std::optional<std::string> (const std::string &)> validator = nullptr) {
        return [=, &var](const std::string &str) -> std::optional<std::string> {
            if (validator) {
                std::optional<std::string> errorMessage = validator(str);
                if (errorMessage) return errorMessage;
            }

            var = str;
            return std::nullopt;
        };
    }

    static ParserFunction boolParser(bool &var) {
        var = false;
        return [=, &var](const std::string &str) -> std::optional<std::string> {
            var = true;
            return std::nullopt;
        };
    }

    ArgumentParser &setProgramDescription(const std::string &programDescription);
    ArgumentParser &addOption(const std::string &name,
                              const std::string &alias,
                              const std::string &valueName,
                              const std::string &description,
                              ParserFunction parserFunction = nullptr,
                              bool optional = false,
                              const std::string &defaultValue = "");
    ArgumentParser &addPositional(const std::string &valueName,
                                  const std::string &description,
                                  ParserFunction parserFunction = nullptr,
                                  bool optional = false,
                                  const std::string &defaultValue = "");
    ArgumentParser &showHelp();
    ArgumentParser &parse();
};

#endif // _MENCI_ARGUMENTPARSER_H
