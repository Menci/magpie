#include <string>
#include <signal.h>
#include <tins/tins.h>

#include "Arguments.h"
#include "Logger.h"
#include "Interface.h"
#include "Sniffer.h"
#include "RouteManager.h"
#include "NDP.h"

void exitOnSignal(int) {
    exit(0);
}

int main(int argc, char *argv[]) {
    auto arguments = parseArguments(argc, argv);

    Logger::initialize(arguments.logLevel);

    for (const auto &interfaceName : arguments.interfaces)
        Interface::initialize(interfaceName);

    Sniffer::initialize();

    signal(SIGINT, exitOnSignal);
    signal(SIGTERM, exitOnSignal);
    signal(SIGQUIT, exitOnSignal);
    signal(SIGHUP, exitOnSignal);

    RouteManager::initialize(
        arguments.alarmInterval,
        arguments.routeProbeInterval,
        arguments.routeProbeRetries,
        arguments.routesSaveFile,
        [] (Tins::IPv6Address address, std::shared_ptr<Interface> interface) {
            auto newPacket = makeNeighborSolicitation(*interface, address);
            Tins::PacketSender(interface->tinsInterface).send(newPacket);
        }
    );

    Sniffer::mainLoop();
}
