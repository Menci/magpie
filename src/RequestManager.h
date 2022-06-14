#pragma once

#include <ctime>
#include <memory>
#include <map>
#include <unordered_map>
#include <tins/tins.h>

#include "Interface.h"

class RequestManager {
    struct NDPRequest {
        Tins::HWAddress<6> sourceMacAddress;
        Tins::IPv6Address sourceAddress;
        Tins::IPv6Address targetAddress;
        std::shared_ptr<Interface> fromInterface;
        time_t requestTime;

        std::unordered_multimap<Tins::IPv6Address, std::shared_ptr<NDPRequest>>::iterator itR;
        std::multimap<time_t, std::shared_ptr<NDPRequest>>::iterator itE;
    };

    static std::unordered_multimap<Tins::IPv6Address, std::shared_ptr<NDPRequest>> requests;
    static std::multimap<time_t, std::shared_ptr<NDPRequest>> requestExperiation;

    static void checkExpiration();
    static void deleteRequest(std::shared_ptr<NDPRequest> request);
    
public:
    static void addRequest(
        const Tins::HWAddress<6> &sourceMacAddress,
        const Tins::IPv6Address &sourceAddress,
        const Tins::IPv6Address &targetAddress,
        std::shared_ptr<Interface> fromInterface
    );
    static void matchAndRespond(
        const Tins::IPv6Address &targetAddress,
        std::function<void (
            Tins::HWAddress<6> sourceMacAddress,
            Tins::IPv6Address sourceAddress,
            std::shared_ptr<Interface> fromInterface
        )> sendPacket
    );
};
