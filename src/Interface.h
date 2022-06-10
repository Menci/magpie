#pragma once

#include <string>
#include <tins/tins.h>

#include "Utils.h"

struct Interface {
    std::string name;
    Tins::NetworkInterface tinsInterface;
    Tins::IPv6Address linkLocal;

    Interface(const std::string &name) :
      name(name),
      tinsInterface(name),
      linkLocal(getLinkLocal(tinsInterface))
    {}
};
