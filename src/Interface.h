#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <tins/tins.h>

#include "Utils.h"

struct Interface {
    std::string name;
    Tins::NetworkInterface tinsInterface;
    Tins::IPv6Address linkLocal;

    static std::unordered_map<std::string, std::shared_ptr<Interface>> interfaces;

    Interface(const std::string &name);

    static void initialize(const std::string &interfaceName);
};
