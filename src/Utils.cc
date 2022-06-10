#include <cstdio>

#include "Utils.h"

std::string toHex(const void *ptr, size_t size) {
    auto sptr = static_cast<const unsigned char *>(ptr);

    std::string result(size * 2, '0');
    for (size_t i = 0; i < size; i++)
        sprintf(&result[i * 2], "%02x", sptr[i]);
    return result;
}

Tins::IPv6Address getLinkLocal(const Tins::NetworkInterface &interface) {
    const static auto LINK_LOCAL_TEMPLATE = Tins::IPv6Address("fe80::AABB:CCff:feDD:EEFF");

    auto ip = LINK_LOCAL_TEMPLATE;
    auto mac = interface.hw_address();

    *(ip.end() - 1) = *(mac.end() - 1);
    *(ip.end() - 2) = *(mac.end() - 2);
    *(ip.end() - 3) = *(mac.end() - 3);

    *(ip.end() - 2 - 4) = *(mac.end() - 4);
    *(ip.end() - 2 - 5) = *(mac.end() - 5);
    *(ip.end() - 2 - 6) = *(mac.end() - 6);

    return ip;
}

bool isLinkLocal(const Tins::IPv6Address &address) {
    auto p = address.begin();
    return p[0] == 0xfe && p[1] == 0x80;
}
