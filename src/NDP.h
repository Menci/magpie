#pragma once

#include <tins/tins.h>

#include "Interface.h"
#include "tins/ipv6_address.h"

Tins::EthernetII makeNeighborSolicitation(const Interface &sendTo, const Tins::IPv6Address &target);
Tins::EthernetII makeNeighborAdvertisement(const Interface &sendTo, const Tins::HWAddress<6> &destMac, const Tins::IPv6Address &destIp, const Tins::IPv6Address &target, bool solicited);
