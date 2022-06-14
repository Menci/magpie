#include "RequestManager.h"

#include "Logger.h"
#include <utility>

std::unordered_multimap<Tins::IPv6Address, std::shared_ptr<RequestManager::NDPRequest>> RequestManager::requests;
std::multimap<time_t, std::shared_ptr<RequestManager::NDPRequest>> RequestManager::requestExperiation;

void RequestManager::checkExpiration() {
    constexpr size_t REQUEST_EXPIRATION_TIME = 10;

    auto now = std::time(nullptr);
    for (auto it = requestExperiation.begin(), next = it; it != requestExperiation.end(); it = next) {
        next = std::next(it);

        auto request = it->second;
        if (now - request->requestTime >= REQUEST_EXPIRATION_TIME) {
            Logger::verbose("deleting expired request for {} from [{}] {}", request->targetAddress, request->fromInterface->name, request->sourceAddress);
            deleteRequest(request);
        } else
            break;
    }
}

void RequestManager::deleteRequest(std::shared_ptr<NDPRequest> request) {
    requests.erase(request->itR);
    requestExperiation.erase(request->itE);
}

void RequestManager::addRequest(
    const Tins::HWAddress<6> &sourceMacAddress,
    const Tins::IPv6Address &sourceAddress,
    const Tins::IPv6Address &targetAddress,
    std::shared_ptr<Interface> fromInterface
) {
    // Check duplicate requests
    auto [begin, end] = requests.equal_range(targetAddress);
    for (auto it = begin; it != end; it++) {
        auto request = it->second;
        if (
            request->sourceMacAddress == sourceMacAddress &&
            request->sourceAddress == sourceAddress &&
            request->fromInterface == fromInterface
        ) {
            deleteRequest(request);
            break;
        }
    }

    auto now = std::time(nullptr);
    auto request = std::make_shared<NDPRequest>();
    request->sourceMacAddress = sourceMacAddress;
    request->sourceAddress = sourceAddress;
    request->targetAddress = targetAddress;
    request->fromInterface = fromInterface;
    request->requestTime = now;
    request->itR = requests.insert(std::make_pair(targetAddress, request));
    request->itE = requestExperiation.insert(std::make_pair(now, request));

    checkExpiration();
}

void RequestManager::matchAndRespond(
    const Tins::IPv6Address &targetAddress,
    std::function<void (
        Tins::HWAddress<6> sourceMacAddress,
        Tins::IPv6Address sourceAddress,
        std::shared_ptr<Interface> fromInterface
    )> sendPacket
) {
    auto [begin, end] = requests.equal_range(targetAddress);
    for (auto it = begin, next = begin; it != end; it = next) {
        next = std::next(it);

        auto request = it->second;
        sendPacket(request->sourceMacAddress, request->sourceAddress, request->fromInterface);
        deleteRequest(request);
    }

    checkExpiration();
}
