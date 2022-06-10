#include <cstdint>
#include <memory>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <tins/tins.h>

#include "Ensure/Ensure.h"
#include "Arguments.h"
#include "Logger.h"
#include "Queue.h"
#include "Utils.h"
#include "Interface.h"
#include "NeighborSolicitation.h"
#include "RouteManager.h"

Queue<std::pair<const std::shared_ptr<Interface>, std::unique_ptr<Tins::PDU>>> queue;
std::map<std::string, std::shared_ptr<Interface>> interfaces;

void onPacket(std::shared_ptr<Interface> interface, Tins::PDU &pdu) {
    auto &eth = pdu.rfind_pdu<Tins::EthernetII>();
    auto &ip6 = pdu.rfind_pdu<Tins::IPv6>();
    auto &icmp6 = pdu.rfind_pdu<Tins::ICMPv6>();

    Logger::debug("packet from {}", interface->name);
    Logger::debug("ETH {} -> {}", eth.src_addr(), eth.dst_addr());
    Logger::debug("IP6 {} -> {}", ip6.src_addr(), ip6.dst_addr());

    if (icmp6.type() == Tins::ICMPv6::NEIGHBOUR_SOLICIT || icmp6.type() == Tins::ICMPv6::NEIGHBOUR_ADVERT) {
        // Ignore link-local address 
        if (isLinkLocal(icmp6.target_addr())) {
            Logger::debug("link-local address {} ignored", icmp6.target_addr());
            return;
        }

        if (icmp6.type() == Tins::ICMPv6::NEIGHBOUR_SOLICIT) {
            Logger::debug("NS target {}", icmp6.target_addr());
            for (auto option : icmp6.options()) {
                Logger::debug("NS Option {}: {}", (int)option.option(), toHex(option.data_ptr(), option.data_size()));
            }

            for (const auto &[name, forwardTo] : interfaces) {
                if (name == interface->name) continue;

                auto newPacket = makeNeighborSolicitation(*forwardTo, icmp6.target_addr());
                Tins::PacketSender(forwardTo->tinsInterface).send(newPacket);

                Logger::verbose("NS forwarded from [{}] to [{}]: {}", interface->name, forwardTo->name, icmp6.target_addr());
            }        
        } else {
            Logger::debug("NA target {}", icmp6.target_addr());        
            
            RouteManager::addOrRefreshRoute(icmp6.target_addr(), interface);

            for (const auto &[name, forwardTo] : interfaces) {
                if (name == interface->name) continue;

                eth.src_addr(forwardTo->tinsInterface.hw_address());
                ip6.src_addr(forwardTo->linkLocal);
                Tins::PacketSender(forwardTo->name).send(pdu);

                Logger::verbose("NA forwarded from [{}] to [{}]: {}", interface->name, forwardTo->name, icmp6.target_addr());
            }
        }
    } else if (icmp6.type() == Tins::ICMPv6::DEST_UNREACHABLE) {
        auto &raw = icmp6.rfind_pdu<Tins::RawPDU>();
        auto payload = raw.payload();

        // The target address could be found in the payload, decoded as the "original IPv6 header"
        // But Tins doesn't provide a way to decode it, so I found the offset by myself
        // It's in the 24-th byte in DU's payload
        constexpr size_t DU_PAYLOAD_START_TARGET = 24;

        if (payload.size() < DU_PAYLOAD_START_TARGET + Tins::IPv6Address::address_size) {
            Logger::warning("malformed DU packet: {}", toHex(payload.data(), payload.size()));
            return;
        }

        auto target = Tins::IPv6Address(static_cast<const uint8_t *>(payload.data() + DU_PAYLOAD_START_TARGET));
        
        // Ignore link-local address 
        if (isLinkLocal(target)) {
            Logger::debug("link-local address {} ignored", target);
            return;
        }

        auto code = icmp6.code();
        Logger::debug("DU code {}, target {}", code, target);

        // Handle "No route to destination" and "Address unreachable" code
        if (!(code == 0 || code == 3)) return;

        for (const auto &[name, forwardTo] : interfaces) {
            if (name == interface->name) continue;

            auto newPacket = makeNeighborSolicitation(*forwardTo, target);
            Tins::PacketSender(forwardTo->tinsInterface).send(newPacket);

            Logger::verbose("DU sending new NS from [{}] to [{}]: {}", interface->name, forwardTo->name, target);
        }
    }
}

void mainLoop() {
    while (true) {
        auto [interface, pdu] = queue.pop();
        try {
            onPacket(interface, *pdu);
        } catch (const Tins::pdu_not_found &e) {
            Logger::error("failed to decode packet with tins: {}", e.what());
        }
    }
}

void startSniffer(const std::string &interfaceName) {
    std::thread([interfaceName] {
        try {
            auto iface = std::make_shared<Interface>(interfaceName);
            auto macAddress = iface->tinsInterface.hw_address().to_string();
            Logger::info("listening on interface: {} [{}]", iface->name, macAddress);
            interfaces[iface->name] = iface;

            static const auto FILTER = (
                "icmp6 and ("
                    // NS or NA, NOT send from this host
                    "((ip6[40] = 135 or ip6[40] = 136) and not ether src {0}) or "
                    // DU, send from this host
                    "((ip6[40] = 1) and ether src {0})"
                ")"
            );
            auto filter = fmt::format(FILTER, macAddress);
            Logger::info("pcap filter '{}'", filter);

            Tins::Sniffer sniffer(interfaceName);
            ENSURE(sniffer.set_filter(filter));
            sniffer.sniff_loop([&] (Tins::PDU &pdu) {
                queue.push(std::make_pair(iface, std::unique_ptr<Tins::PDU>(pdu.clone())));
                return true;
            });
        } catch (const Tins::invalid_interface &) {
            Logger::error("invalid interface {}", interfaceName);
            exit(1);
        }
    }).detach();
}

void exitOnSignal(int) {
    exit(0);
}

int main(int argc, char *argv[]) {
    auto arguments = parseArguments(argc, argv);

    Logger::initialize(arguments.logLevel);

    for (const auto &interfaceName : arguments.interfaces)
        startSniffer(interfaceName);

    signal(SIGINT, exitOnSignal);
    signal(SIGTERM, exitOnSignal);
    signal(SIGQUIT, exitOnSignal);
    signal(SIGHUP, exitOnSignal);

    RouteManager::initialize(
        arguments.alarmInterval,
        arguments.routeProbeInterval,
        arguments.routeProbeRetries,
        arguments.preserveRoutesOnExit,
        [] (Tins::IPv6Address address, std::shared_ptr<Interface> interface) {
            auto newPacket = makeNeighborSolicitation(*interface, address);
            Tins::PacketSender(interface->tinsInterface).send(newPacket);
        }
    );

    mainLoop();
}
