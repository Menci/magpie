#pragma once

#include <tins/tins.h>

#include "Interface.h"

Tins::EthernetII makeNeighborSolicitation(const Interface &sendTo, const Tins::IPv6Address &target);
