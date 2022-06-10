#include "ArgumentParser.h"

ArgumentParser &ArgumentParser::setProgramDescription(const std::string &programDescription) {
    this->programDescription = programDescription;
    return *this;
}

ArgumentParser &ArgumentParser::addOption(const std::string &name,
                                          const std::string &alias,
                                          const std::string &valueName,
                                          const std::string &description,
                                          ParserFunction parserFunction,
                                          bool optional,
                                          const std::string &defaultValue) {
    if (mapNameArgument.count(name) > 0) {
        throw std::logic_error("Duplicated argument name: " + name);
    }

    if (mapAliasArgument.count(alias) > 0) {
        throw std::logic_error("Duplicated argument alias: " + alias);
    }

    if (!optional && valueName.length() == 0) {
        throw std::logic_error("Non-optional option must receive a value: --" + name);
    }

    std::shared_ptr<Argument> arg = std::make_shared<Argument>();
    arg->name = name;
    arg->alias = alias;
    arg->valueName = valueName;
    arg->description = description;
    arg->parserFunction = parserFunction;
    arg->optional = optional;
    arg->specfied = false;
    arg->defaultValue = defaultValue;

    mapNameArgument[name] = arg;
    if (alias.length() > 0) {
        mapAliasArgument[alias] = arg;
    }
    options.push_back(arg);

    return *this;
}

ArgumentParser &ArgumentParser::addPositional(const std::string &valueName,
                                              const std::string &description,
                                              ParserFunction parserFunction,
                                              bool optional,
                                              const std::string &defaultValue) {
    if (!positionalArguments.empty() && positionalArguments.back().optional && !optional) {
        throw std::logic_error("Non-optional positional argument after optional is not allowed: " + valueName);
    }

    Argument arg;
    arg.valueName = valueName;
    arg.description = description;
    arg.parserFunction = parserFunction;
    arg.optional = optional;
    arg.specfied = false;
    arg.defaultValue = defaultValue;

    positionalArguments.push_back(arg);

    return *this;
}

ArgumentParser &ArgumentParser::showHelp() {
    // Print a line like '  --help       Show this help message.'.
    auto printArgumentDescription = [](const std::string &argument, const std::string &description, const std::string &defaultValue = "") {
        const size_t argumentNameLength = 40;
        const size_t lineLimit = 150;

        std::clog << "  " << argument;
        // Pad to argumentNameLength characters.
        if (argumentNameLength > argument.length() - 2) {
            size_t spaceCount = argumentNameLength - (argument.length() - 2);
            std::clog << std::string(spaceCount, ' ');
        }

        std::clog << description;

        if (defaultValue.length() > 0) {
            if (argumentNameLength + description.length() + defaultValue.length() > lineLimit) {
                std::clog << std::endl
                          << std::string(argumentNameLength, ' ') << "Default: " << defaultValue;
            } else {
                std::clog << " (Default: " << defaultValue << ')';
            }
        }

        std::clog << std::endl;
    };

    // Generate a string like '[-o <file>]'.
    auto displayOptionUsage = [](const Argument &arg) -> std::string {
        std::string str = arg.alias.length() > 0 ? '-' + arg.alias
                                              : "--" + arg.name;
        if (arg.valueName.length() > 0) str += arg.alias.length() > 0 ? " <" + arg.valueName + '>'
                                                                      : "=<" + arg.valueName + '>';
        if (arg.optional) str = '[' + str + ']';
        return str;
    };

    // Begin usage.
    std::clog << "Usage: " << argv[0];
    for (std::shared_ptr<Argument> arg : options) {
        std::clog << ' ' << displayOptionUsage(*arg);
    }
    for (Argument &arg : positionalArguments) {
        std::clog << " <" << arg.valueName << '>';
    }
    std::clog << std::endl;

    std::clog << "       " << argv[0] << " -?" << std::endl;
    std::clog << std::endl;
    // End Usage.

    // Begin program description.
    std::clog << programDescription << std::endl;
    std::clog << std::endl;
    // End program description.

    // Begin positional arguments's description.
    if (!positionalArguments.empty()) {
        std::clog << "Positional arguments:" << std::endl;
        for (Argument &arg : positionalArguments) {
            printArgumentDescription('<' + arg.valueName + '>', arg.description);
        }
        std::clog << std::endl;
    }
    // End positional arguments's description.

    // Generate a string like '-o, --output=<file>'.
    auto displayOptionName = [](const Argument &arg) -> std::string {
        std::string str;
        if (arg.alias.length() > 0) {
            str += '-' + arg.alias + ", "; // 4 characters.
        } else str += "    ";

        str += "--" + arg.name;
        if (arg.valueName.length() > 0) {
            str += "=<" + arg.valueName + '>';
        }

        return str;
    };

    std::clog << "Options:" << std::endl;
    printArgumentDescription("-?, --help",
                             "Show this help message and exit.");
    for (std::shared_ptr<Argument> arg : options) {
        printArgumentDescription(displayOptionName(*arg),
                                 arg->description,
                                 arg->valueName.length() > 0 ? arg->defaultValue : "");
    }

    return *this;
}

ArgumentParser &ArgumentParser::parse() {
    auto raiseError = [&](const std::string &message) {
        std::clog << message << std::endl;
        std::clog << "Try '" << argv[0] << " -?' for more information." << std::endl;
        exit(2); // USAGE
    };

    // Skip any string like option (starts with "-") after a "--".
    bool skipAnyLikeOption = false;
    size_t positionalIndex = 0;
    for (size_t i = 1; i < argc; i++) {
        const std::string curr = argv[i];

        std::shared_ptr<Argument> arg;
        std::string name;

        if (curr.rfind("-", 0) == 0 && !skipAnyLikeOption) {
            if (curr == "--") {
                // Skip all later arguments like option.
                skipAnyLikeOption = true;
                continue;
            }

            // Check if calling help.
            if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-?") {
                showHelp();
                exit(0);
            }

            // Option.
            // Both "--option value" and "--option=value" is allowed.
            // Both "-ovalue" and "-o value" is allowed.
            bool valueGot = false;
            std::string value;
            if (curr.rfind("--", 0) == 0) {
                size_t equalSignPos = curr.find('=');
                name = equalSignPos == std::string::npos ? curr : curr.substr(0, equalSignPos);
                if (equalSignPos != std::string::npos) {
                    // Value passed with "--option=value".
                    value = curr.substr(equalSignPos + 1);
                    valueGot = true;
                }

                auto it = mapNameArgument.find(name.substr(2));
                if (it == mapNameArgument.end()){
                    raiseError("Unknown option: " + name + '.');
                }
                arg = it->second;
            } else {
                name = curr.length() == 2 ? curr : curr.substr(0, 2);
                if (curr.length() > 2) {
                    // Value passed with "-ovalue".
                    value = curr.substr(2);
                    valueGot = true;
                }

                auto it = mapAliasArgument.find(name.substr(1));
                if (it == mapAliasArgument.end()){
                    raiseError("Unknown option: " + name + '.');
                }
                arg = it->second;
            }

            arg->specfied = true;
            if (arg->valueName.length() == 0) {
                // Only option, no value.
                if (arg->parserFunction) arg->parserFunction("");
                continue;
            }

            // Get value.
            if (!valueGot) {
                // Value passed with "--option value" or "-o value".
                if (i + 1 >= argc) {
                    raiseError("Missing value for option: " + name);
                }

                value = argv[++i];
            }

            if (arg->parserFunction) {
                std::optional<std::string> errorMessage = arg->parserFunction(value);
                if (errorMessage) {
                    raiseError("Invalid value \"" + value + "\" for option " + name + ": " + *errorMessage);
                }
            }
        } else {
            // Positional argument.
            if (positionalIndex == positionalArguments.size()) {
                raiseError("Too many arguments: \"" + curr + "\" is extra.");
            }

            Argument &arg = positionalArguments[positionalIndex++];
            
            arg.specfied = true;

            if (arg.parserFunction) {
                std::optional<std::string> errorMessage = arg.parserFunction(curr);
                if (errorMessage) {
                    raiseError("Invalid value \"" + curr + "\" for positional argument <" + arg.valueName + ">: " + *errorMessage);
                }
            }
        }
    }

    // argv processed, use default value for non-specfied.
    auto useDefaultValueForUnspecfied = [&](Argument &arg, bool isOption) {
        if (!arg.specfied && !arg.valueName.empty()) {
            if (!arg.optional) {
                if (isOption) {
                    raiseError("Missing value for non-optional option: --" + arg.name + ".");
                } else {
                    raiseError("Missing value for non-optional positional argument: <" + arg.valueName + ">.");
                }
            }

            const std::string &value = arg.defaultValue;
            if (arg.parserFunction) {
                std::optional<std::string> errorMessage = arg.parserFunction(value);
                if (errorMessage) {
                    if (isOption) {
                        raiseError("Invalid default value \"" + value + "\" for option --" + arg.name + " <" + arg.valueName + ">: " + *errorMessage);
                    } else {
                        raiseError("Invalid default value \"" + value + "\" for positional argument <" + arg.valueName + ">: " + *errorMessage);
                    }
                }
            }
        }
    };

    for (Argument &arg : positionalArguments) {
        useDefaultValueForUnspecfied(arg, false);
    }

    for (std::shared_ptr<Argument> &arg : options) {
        useDefaultValueForUnspecfied(*arg, true);
    }

    return *this;
}
