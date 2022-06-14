#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <ctime>
#include <map>
#include <unordered_map>
#include <tins/tins.h>

#include "Interface.h"

class RouteManager {
    struct RouteItem {
        Tins::IPv6Address address;
        std::shared_ptr<Interface> interface;
        time_t lastProbe;
        size_t probeRetries;

        std::unordered_map<Tins::IPv6Address, std::shared_ptr<RouteItem>>::iterator itR;
        std::multimap<time_t, std::shared_ptr<RouteItem>>::iterator itE;
    };

    static size_t checkInterval;
    static size_t probeInterval;
    static size_t probeRetries;
    static std::function<void (Tins::IPv6Address, std::shared_ptr<Interface>)> probeCallback;

    static std::unordered_map<Tins::IPv6Address, std::shared_ptr<RouteItem>> routes;
    static std::multimap<time_t, std::shared_ptr<RouteItem>> routeExpiration;

    static void deleteRoute(std::shared_ptr<RouteItem> item);
    static void updateRouteTable(std::shared_ptr<RouteItem> item, bool isAdd);
    static void printManagedRoutes();

    static void setTimer();
    static void processTimerTick(int);
    static void deleteAllRoutesOnExit();

public:
    static void initialize(size_t checkInterval, size_t probeInterval, size_t probeRetries, bool preserveRoutesOnExit, std::function<void (Tins::IPv6Address, std::shared_ptr<Interface>)> probeCallback);
    static void addOrRefreshRoute(const Tins::IPv6Address &address, std::shared_ptr<Interface> interface);
    static std::shared_ptr<Interface> getRoute(const Tins::IPv6Address &address);
};
