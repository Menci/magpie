#pragma once

#include <string>

#include <tins/tins.h>

std::string toHex(const void *ptr, size_t size);
Tins::IPv6Address getLinkLocal(const Tins::NetworkInterface &interface);
bool isLinkLocal(const Tins::IPv6Address &address);
