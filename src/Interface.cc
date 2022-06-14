#include "Interface.h"

#include "Logger.h"

std::unordered_map<std::string, std::shared_ptr<Interface>> Interface::interfaces;

Interface::Interface(const std::string &name) :
    name(name),
    tinsInterface(name),
    linkLocal(getLinkLocal(tinsInterface))
{}

void Interface::initialize(const std::string &interfaceName) {
    try {
        auto interface = std::make_shared<Interface>(interfaceName);
        interfaces[interfaceName] = interface;
    } catch (const Tins::invalid_interface &) {
        Logger::error("invalid interface {}", interfaceName);
        exit(1);
    }
}
