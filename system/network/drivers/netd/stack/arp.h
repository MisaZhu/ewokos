#ifndef ARP_H
#define ARP_H

#include <stdint.h>

#include "net.h"
#include "ip.h"

#define ARP_RESOLVE_ERROR      -1
#define ARP_RESOLVE_INCOMPLETE  0
#define ARP_RESOLVE_FOUND       1

extern int
arp_resolve(struct net_iface *iface, ip_addr_t pa, uint8_t *ha);
/*
 * Park a datagram behind an INCOMPLETE entry; it is transmitted from
 * arp_input() when the reply arrives. Returns -1 if there is no INCOMPLETE
 * entry for pa (caller should retry arp_resolve()) or on allocation failure.
 */
extern int
arp_pending_push(struct net_iface *iface, ip_addr_t pa, const uint8_t *data, size_t len);
/*
 * Broadcast a gratuitous ARP request + reply for the interface's current
 * address, refreshing neighbour caches that still point at the previous
 * owner of a reused DHCP lease. Best effort; returns -1 only if no frame
 * could be queued.
 */
extern int
arp_announce(struct net_iface *iface);
/*
 * Send a directed request for pa even if the cache says RESOLVED, so a
 * neighbour whose MAC changed is refreshed instead of trusted for the full
 * cache lifetime.
 */
extern int
arp_probe(struct net_iface *iface, ip_addr_t pa);
extern int
arp_init(void);

#endif
