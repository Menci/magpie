#include "Interface.h"

#include "Logger.h"

std::unordered_map<std::string, std::shared_ptr<Interface>> Interface::interfaces;

Interface::Interface(const std::string &name) :
    name(name),
    tinsInterface(name),
    linkLocal(getLinkLocal(tinsInterface))
{}

void Interface::initialize(const std::string &interfaceName) {
    if (interfaceName == "lo") {
        Logger::error("refuse to relay on loopback interface!");
        exit(1);
    }

    try {
        auto interface = std::make_shared<Interface>(interfaceName);

        if (interfaces.find(interfaceName) != interfaces.end()) {
            Logger::error("duplicated interface {}", interfaceName);
            exit(1);
        }
        interfaces[interfaceName] = interface;
    } catch (const Tins::invalid_interface &) {
        Logger::error("invalid interface {}", interfaceName);
        exit(1);
    }
}

// Loopback interface is only used for capturing DU packets
Interface::Interface(bool) :
    name("lo"),
    tinsInterface("lo"),
    linkLocal("::1") // unused
{}

std::shared_ptr<Interface> Interface::getLoopback() {
    static std::shared_ptr<Interface> loopback;
    if (!loopback) {
        loopback = std::shared_ptr<Interface>(new Interface(true));
    }

    return loopback;
}
