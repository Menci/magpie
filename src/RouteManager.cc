#include "RouteManager.h"

#include <iostream>
#include <memory>
#include <unistd.h>
#include <fmt/format.h>

#include "Ensure/Ensure.h"
#include "Logger.h"

size_t RouteManager::checkInterval;
size_t RouteManager::probeInterval;
size_t RouteManager::probeRetries;
std::function<void (Tins::IPv6Address, std::shared_ptr<Interface>)> RouteManager::probeCallback;

std::unordered_map<Tins::IPv6Address, std::shared_ptr<RouteManager::RouteItem>> RouteManager::routes;
std::multimap<time_t, std::shared_ptr<RouteManager::RouteItem>> RouteManager::routeExpiration;

void RouteManager::initialize(
    size_t checkInterval,
    size_t probeInterval,
    size_t probeRetries,
    bool preserveRoutesOnExit,
    std::function<void (Tins::IPv6Address, std::shared_ptr<Interface>)> probeCallback
) {
    RouteManager::checkInterval = checkInterval;
    RouteManager::probeInterval = probeInterval;
    RouteManager::probeRetries = probeRetries;
    RouteManager::probeCallback = probeCallback;

    // Set POSIX timer
    ENSURE_ERRNO(signal(SIGALRM, processTimerTick));
    setTimer();

    if (preserveRoutesOnExit) {
        Logger::warning("preserveRoutesOnExit enabled, this may cause routing table resource leak");
    } else {
        // Remove ALL managed routes on exit
        ENSURE_ERRNO(std::atexit(RouteManager::deleteAllRoutesOnExit));
    }
}

void RouteManager::addOrRefreshRoute(const Tins::IPv6Address &address, std::shared_ptr<Interface> interface) {
    // Find old one
    if (auto itR = routes.find(address); itR != routes.end()) {
        auto oldRoute = itR->second;
        if (oldRoute->interface != interface) {
            // Replace -- delete old first
            Logger::warning("host {} moved from interface [{}] to [{}]", address, oldRoute->interface->name, interface->name);
            deleteRoute(oldRoute);
        } else if (oldRoute->interface == interface) {
            // Refresh
            oldRoute->lastProbe = std::time(nullptr);
            oldRoute->probeRetries = 0;
            routeExpiration.erase(oldRoute->itE);
            oldRoute->itE = routeExpiration.insert(std::make_pair(oldRoute->lastProbe, oldRoute));
            return;
        }
    }

    auto route = std::make_shared<RouteItem>();
    route->address = address;
    route->interface = interface;
    route->lastProbe = std::time(nullptr);
    route->probeRetries = 0;
    route->itR = routes.insert(std::make_pair(address, route)).first;
    route->itE = routeExpiration.insert(std::make_pair(route->lastProbe, route));

    updateRouteTable(route, true);
}

std::shared_ptr<Interface> RouteManager::getRoute(const Tins::IPv6Address &address) {
    auto it = routes.find(address);
    if (it != routes.end()) return it->second->interface;
    return nullptr;
}

void RouteManager::deleteRoute(std::shared_ptr<RouteItem> item) {
    updateRouteTable(item, false);
    routes.erase(item->itR);
    routeExpiration.erase(item->itE);
}

void RouteManager::updateRouteTable(std::shared_ptr<RouteItem> item, bool isAdd) {
    auto command = fmt::format("ip -6 route {} {} dev {}", (isAdd ? "add" : "del"), item->address, item->interface->name);
    Logger::info("executing '{}'", command);
    if (system(command.c_str()) != 0) {
        Logger::error("command failed!");
    }
}

void RouteManager::printManagedRoutes() {
    for (const auto [_, route] : routeExpiration) {
        std::string formattedTime = std::ctime(&route->lastProbe);
        formattedTime[formattedTime.length() - 1] = '0'; // Remove tailing '\n'

        Logger::debug("route {} dev {} [last probe = {}, retries = {}]", route->address, route->interface->name, formattedTime, route->probeRetries);
    }
}

void RouteManager::setTimer() {
    ENSURE_ERRNO(alarm(checkInterval));
}

void RouteManager::processTimerTick(int) {
    if (Logger::showLevel >= Logger::DEBUG) printManagedRoutes();

    auto now = std::time(nullptr);
    for (auto it = routeExpiration.begin(), next = it; it != routeExpiration.end(); it = next) {
        next = std::next(it);

        auto route = it->second;
        if (now - route->lastProbe >= probeInterval) {
            if (++route->probeRetries > probeRetries) {
                // Max probe retries reached
                Logger::info("deleting expired route {} dev {}", route->address, route->interface->name);
                deleteRoute(route);
            } else {
                // Retry probe
                routeExpiration.erase(route->itE);
                route->lastProbe = now;
                route->itE = routeExpiration.insert(std::make_pair(route->lastProbe, route));
                Logger::verbose("re-probing route {} dev {}, retry = {}", route->address, route->interface->name, route->probeRetries);
                probeCallback(route->address, route->interface);
            }
        } else
            break;
    }

    setTimer();
}

void RouteManager::deleteAllRoutesOnExit() {
    for (const auto [_, route] : routeExpiration) {
        updateRouteTable(route, false);
    }

    // The process won't exit without this line
    _exit(0);
}
