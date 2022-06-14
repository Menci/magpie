# Magpie

Bidirectional NDP proxy and route maintainer to relay an IPv6 SLAAC network.

> The name "magpie" is from the Chinese folk tale *[The Cowherd and the Weaver Girl](https://en.wikipedia.org/wiki/The_Cowherd_and_the_Weaver_Girl)*: once a year on the 7-th day of the 7-th lunar month, all the magpies flys to the Milky Way and form a ***bridge*** to let couple, the Cowherd and the Weaver Girl, to meet.

## Introduction

In an IPv6 network, the **downstream router** (your homelab LAN gateway) usually got a /64 routed prefix with DHCPv6's PD (prefix delegation) from **upstream network** (ISP). Then your downstream router could broadcast RA with the delegated prefix so that all hosts in your main LAN could get IPv6 addresses with SLAAC.

```plain
        Upstream         <--->       Downstream Router        <--->            Hosts in main LAN
(DHCPv6 server with PD)         (Got /64 PD from upstream)              (Got IPv6 addresses with SLAAC)
                                     (Broadcasting RA)
```

But what if you connect a **secondary router** to the **downstream router**'s main LAN? Your secondary router gets a globally routable IPv6 address with that prefix, but not your secondary LAN's terminal devices because no one is broadcasting RA. You can pretend to be owning that PD, re-broadcasting the **downstream router**'s RA to the secondary LAN (with [radvd](https://radvd.litech.org/)). But even if your terminal devices could get "real" IPv6 address, it's not globally routable.

```plain
     Downstream Router        <--->             Secondary Router             <--->        Terminal devices in secondary LAN
(Got /64 PD from upstream)              (Got IPv6 addresses with SLAAC)                    (Got IPv6 addresses with SLAAC)
     (Broadcasting RA)                        (Re-broadcasting RA)                               (But not working?)
```

Why? Because that /64 prefix doesn't belong to your **secondary router**, an output packet could reach the Internet but no input packet goes to your terminal devices through your **secondary router**. To go deeper, the **upstream router**, who owns the PD in real, doesn't know where to send the packet.

So why it can without the **secondary router**? Just like IPv4's ARP, in IPv6, a host resolves an IP address in the same broadcast domain to MAC address with [NDP](https://en.wikipedia.org/wiki/Neighbor_Discovery_Protocol). But your **secondary router** divides the hosts in a /64 network, which are in theory belong to one broadcast domain, to two -- the main LAN and secondary LAN. Then there are two problems involved:

1. For an IPv6 address used by your secondary LAN's terminal device, the **downstream router**, who need to do the first-time routing for each packet to your network, couldn't find it with NDP.
2. For an IPv6 address in the /64 prefix of all your hosts, your **secondary router** don't know whether it's in main LAN or secondary LAN -- since the two interfaces are in the same broadcast domain in theory.

For the 1st problem, we can proxy the NDP messages on your **secondary router**. When receiving a NDP query (NS) from one network, send a NDP query to the other network from itself, when receving a NDP response (NA) from one network, respond to the the other network to announce the IP is on itself. Then hosts in two networks are thinking they're in the same broadcast domain, but in real they communicate with your **secondary router** as the gateway.

For the 2st problem, we can learn routes from the NDP responses (NA). Once receving a NA from one network, add a rule to route the target IP to that network in the system routing table. We have a timeout and reprobe mechanism to delete expired (disconnected) hosts. In addition, we can capture the output ICMPv6 "destination unreachable" messages and try to probe them and fix the routes.

## Building

Install dependencies:

```bash
apt install -y libpcap-dev # Ubuntu or Debian
pacman -Sy libpcap         # Archlinux
```

Clone this repo with `--recursive`:

```bash
git clone https://github.com/Menci/magpie --recursive
cd magpie
```

Build with CMake:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
# Result binary: ./src/magpie
```

## Usage

Magpie listens on multiple (two normally but more are possible) interfaces, specified by `-i`, and do NDP proxying and routes probing/learning. Usually it's the only argument needed. But for debugging purpose you could also specify the log level with `-l`.

```bash
magpie -i wan,br-lan # Relaying IPv6 on interface "wan" and "br-lan"
magpie -i wan,br-lan -l verbose # Increase the log level for 
```

It sets alarm and check for the timeout of routes in each `--alarm-interval, -a` seconds, any route lasted `--probe-interval, -p` seconds will be reprobed. There will be `--probe-retries, -r` reprobe retries before a route being deleted as expired. For example, the default:

```bash
# A route will be reprobed 5 times before deleted as expired, in an interval of 60s for each reprobe
magpie -i wan,br-lan -a 10 -p 60 -r 5
```

It's better to provide a `--routes-save-file` to save the routes to file on exit and load (reprobe) them on start. This helps reduce the IPv6 network down time between your restarts of the daemon.

```bash
magpie -i wan,br-lan -f /var/lib/magpie/saved-routes.json
```

## Security Notice

This project aims on using in homelab / school network in which the hosts are trusted. **Don't use it in a public / untrusted network** since it maintains routing states without any security measure. Attacks like NDP hijacking and routing table DDoS could be done easily.
