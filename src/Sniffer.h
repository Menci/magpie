#pragma once

#include <string>
#include <memory>
#include <tins/tins.h>

#include "Queue.h"
#include "Interface.h"

class Sniffer {
    static Queue<std::pair<const std::shared_ptr<Interface>, std::unique_ptr<Tins::PDU>>> queue;

    static void onPacket(std::shared_ptr<Interface> interface, Tins::PDU &pdu);
    static void startOnInterface(std::shared_ptr<Interface> interface, const std::string &filterExceptLocalMacAddresses);

public:
    static void initialize();
    static void mainLoop();
};
