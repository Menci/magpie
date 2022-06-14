#include "Sniffer.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <fmt/format.h>

#include "Ensure/Ensure.h"
#include "Logger.h"
#include "NDP.h"
#include "RouteManager.h"
#include "RequestManager.h"

Queue<std::pair<const std::shared_ptr<Interface>, std::unique_ptr<Tins::PDU>>> Sniffer::queue;

void Sniffer::onPacket(std::shared_ptr<Interface> interface, Tins::PDU &pdu) {
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
            Logger::verbose("NS target {}", icmp6.target_addr());
            for (auto option : icmp6.options()) {
                Logger::debug("NS Option {}: {}", (int)option.option(), toHex(option.data_ptr(), option.data_size()));
            }

            auto onInterface = RouteManager::getRoute(icmp6.target_addr());
            if (onInterface && onInterface != interface) {
                // Reply
                auto newPacket = makeNeighborAdvertisement(*interface, eth.src_addr(), ip6.src_addr(), icmp6.target_addr(), true);
                Tins::PacketSender(interface->tinsInterface).send(newPacket);
                
                Logger::verbose("NS replied with unicast NA");
            } else if (!onInterface) {
                // Save to request manager for later respond
                RequestManager::addRequest(
                    eth.src_addr(),
                    ip6.src_addr(),
                    icmp6.target_addr(),
                    interface
                );

                // Forward NS to other interfaces
                for (const auto &[name, forwardTo] : Interface::interfaces) {
                    if (name == interface->name) continue;

                    auto newPacket = makeNeighborSolicitation(*forwardTo, icmp6.target_addr());
                    Tins::PacketSender(forwardTo->tinsInterface).send(newPacket);

                    Logger::verbose("NS forwarded from [{}] to [{}]: {}", interface->name, forwardTo->name, icmp6.target_addr());
                }
            }
        } else {
            Logger::verbose("NA target {}", icmp6.target_addr());
            for (auto option : icmp6.options()) {
                Logger::debug("NA Option {}: {}", (int)option.option(), toHex(option.data_ptr(), option.data_size()));
            }

            RouteManager::addOrRefreshRoute(icmp6.target_addr(), interface);

            // Forward multicast NA to other interfaces
            if (ip6.dst_addr().is_multicast()) {
                Logger::info("ETH {} -> {}", eth.src_addr(), eth.dst_addr());
                Logger::info("IP6 {} -> {}", ip6.src_addr(), ip6.dst_addr());
                for (const auto &[name, forwardTo] : Interface::interfaces) {
                    if (name == interface->name) continue;

                    auto newPacket = makeNeighborAdvertisement(*interface, eth.dst_addr(), ip6.dst_addr(), icmp6.target_addr(), false);
                    Tins::PacketSender(forwardTo->tinsInterface).send(newPacket);

                    Logger::verbose("multicast NA forwarded from [{}] to [{}]: {}", interface->name, forwardTo->name, icmp6.target_addr());
                }
            }

            // Reply to earlier requests
            RequestManager::matchAndRespond(icmp6.target_addr(), [&] (Tins::HWAddress<6> sourceMacAddress, Tins::IPv6Address sourceAddress, std::shared_ptr<Interface> fromInterface) {
                auto newPacket = makeNeighborAdvertisement(*fromInterface, sourceMacAddress, sourceAddress, icmp6.target_addr(), true);
                Tins::PacketSender(fromInterface->tinsInterface).send(newPacket);
               
                Logger::info("responded NA to NS for {} from [{}] {}", icmp6.target_addr(), interface->name, sourceAddress);
            });
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
        Logger::verbose("DU code {}, target {}", code, target);

        // Handle "No route to destination" and "Address unreachable" code
        if (!(code == 0 || code == 3)) return;

        for (const auto &[name, forwardTo] : Interface::interfaces) {
            if (name == interface->name) continue;

            auto newPacket = makeNeighborSolicitation(*forwardTo, target);
            Tins::PacketSender(forwardTo->tinsInterface).send(newPacket);

            Logger::verbose("DU sending new NS from [{}] to [{}]: {}", interface->name, forwardTo->name, target);
        }
    }
}

void Sniffer::initialize() {
    std::string filterLocalMacAddresses;
    for (auto [_, interface] : Interface::interfaces) {
        if (!filterLocalMacAddresses.empty()) filterLocalMacAddresses += " or ";
        filterLocalMacAddresses += fmt::format("ether src {}", interface->tinsInterface.hw_address());
    }
    auto filterExceptLocalMacAddresses = fmt::format("not ({})", filterLocalMacAddresses);

    for (auto [_, interface] : Interface::interfaces)
        startOnInterface(interface, filterExceptLocalMacAddresses);
}

void Sniffer::startOnInterface(std::shared_ptr<Interface> interface, const std::string &filterExceptLocalMacAddresses) {
    std::mutex mutex;
    std::condition_variable cv;
    bool started = false;

    std::thread([&, interface] {
        auto macAddress = interface->tinsInterface.hw_address().to_string();
        Logger::info("listening on interface: {} [{}]", interface->name, macAddress);

        static const auto FILTER = (
            "icmp6 and ("
                // NS or NA, NOT send from this host
                "((ip6[40] = 135 or ip6[40] = 136) and {0}) or "
                // DU, send from this host
                "((ip6[40] = 1) and ether src {1})"
            ")"
        );
        auto filter = fmt::format(FILTER, filterExceptLocalMacAddresses, macAddress);
        Logger::info("pcap filter '{}'", filter);

        Tins::Sniffer sniffer(interface->name);
        ENSURE(sniffer.set_filter(filter));

        // Notify started
        {
            std::lock_guard lock(mutex);
            started = true;
            cv.notify_one();
        }

        // Enter loop
        sniffer.sniff_loop([&] (Tins::PDU &pdu) {
            queue.push(std::make_pair(interface, std::unique_ptr<Tins::PDU>(pdu.clone())));
            return true;
        });
    }).detach();

    // Wait for started
    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return started; });
    }
}

void Sniffer::mainLoop() {
    while (true) {
        auto [interface, pdu] = queue.pop();
        try {
            onPacket(interface, *pdu);
        } catch (const Tins::pdu_not_found &e) {
            Logger::error("failed to decode packet with tins: {}", e.what());
        }
    }
}
