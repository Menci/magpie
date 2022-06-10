#include "NeighborSolicitation.h"

#include <random>

Tins::EthernetII makeNeighborSolicitation(const Interface &sendTo, const Tins::IPv6Address &target) {
    const static auto NS_TARGET_MAC_TEMPLATE = Tins::HWAddress<6>("33:33:ff:AA:BB:CC");
    auto destMac = NS_TARGET_MAC_TEMPLATE;
    for (size_t i = 1; i <= 3; i++) destMac[destMac.address_size - i] = *(target.end() - i);

    const static auto NS_TARGET_IP_TEMPLATE = Tins::IPv6Address("ff02::1:ffAA:BBCC");
    auto destIp = NS_TARGET_IP_TEMPLATE;
    for (size_t i = 1; i <= 3; i++) *(destIp.end() - i) = *(target.end() - i);

    auto sourceMac = sendTo.tinsInterface.hw_address();
    
    auto eth = Tins::EthernetII(destMac, sourceMac);
    auto ip6 = Tins::IPv6(destIp, sendTo.linkLocal);
    auto icmp6 = Tins::ICMPv6(Tins::ICMPv6::NEIGHBOUR_SOLICIT);

    static std::mt19937 flowLabelGenerator(std::random_device{}());
    ip6.flow_label(flowLabelGenerator() & decltype(ip6.flow_label())::max_value);
    ip6.hop_limit(255);

    icmp6.target_addr(target);
    icmp6.add_option(Tins::ICMPv6::option(Tins::ICMPv6::SOURCE_ADDRESS, sourceMac.address_size, sourceMac.begin()));

    return eth / ip6 / icmp6;
}
