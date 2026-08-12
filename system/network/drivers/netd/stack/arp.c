#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../platform.h"

#include "util.h"
#include "net.h"
#include "ether.h"
#include "arp.h"
#include "ip.h"
//#define debugf errorf
/* see https://www.iana.org/assignments/arp-parameters/arp-parameters.txt */
#define ARP_HRD_ETHER 0x0001
/* NOTE: use same value as the Ethernet types */
#define ARP_PRO_IP ETHER_TYPE_IP

#define ARP_OP_REQUEST 0x0001
#define ARP_OP_REPLY   0x0002

#define ARP_CACHE_SIZE 32
#define ARP_CACHE_TIMEOUT 600 /* seconds */
/*
 * An entry that never gets a reply must not live for ARP_CACHE_TIMEOUT: it
 * would keep every later ip_output() to that address in the INCOMPLETE path
 * and pin one of the 32 slots.
 */
#define ARP_INCOMPLETE_TIMEOUT 5 /* seconds */
/* Do not re-broadcast a request for the same target more often than this. */
#define ARP_REQUEST_INTERVAL_US 500000
/*
 * Global cap on ARP replies: arp_input() used to answer every who-has
 * targeting our IP unconditionally, so an ARP flood on the LAN turned
 * this host into a 1:1 amplifier (each inbound broadcast produced an
 * outbound reply) and jammed the wlan TX path. Normal peers re-resolve
 * at most every few minutes, so a 20 replies/s ceiling never affects
 * legitimate traffic but bounds the amplification hard.
 */
#define ARP_REPLY_WINDOW_US 100000
#define ARP_REPLY_MAX_PER_WINDOW 2
/* One MTU-sized datagram may wait per unresolved neighbour. */
#define ARP_PENDING_MAX 2048

#define ARP_CACHE_STATE_FREE       0
#define ARP_CACHE_STATE_INCOMPLETE 1
#define ARP_CACHE_STATE_RESOLVED   2
#define ARP_CACHE_STATE_STATIC     3

struct arp_hdr {
    uint16_t hrd;
    uint16_t pro;
    uint8_t hln;
    uint8_t pln;
    uint16_t op;
};

struct arp_ether {
    struct arp_hdr hdr;
    uint8_t sha[ETHER_ADDR_LEN];
    uint8_t spa[IP_ADDR_LEN];
    uint8_t tha[ETHER_ADDR_LEN];
    uint8_t tpa[IP_ADDR_LEN];
};

struct arp_cache {
    unsigned char state;
    ip_addr_t pa;
    uint8_t ha[ETHER_ADDR_LEN];
    struct timeval timestamp;
    /* pending datagram queued while the entry is INCOMPLETE */
    struct net_iface *iface;
    struct timeval request;
    uint8_t *pending;
    size_t pending_len;
};

static mutex_t mutex = MUTEX_INITIALIZER;
static struct arp_cache caches[ARP_CACHE_SIZE];

static char *
arp_opcode_ntoa(uint16_t opcode)
{
    switch (ntoh16(opcode)) {
    case ARP_OP_REQUEST:
        return "Request";
    case ARP_OP_REPLY:
        return "Reply";
    }
    return "Unknown";
}

static void
arp_dump(const uint8_t *data, size_t len)
{
#ifdef NET_DEBUG
    struct arp_ether *message;
    ip_addr_t spa, tpa;
    char addr[128];

    message = (struct arp_ether *)data;
    infof( "        hrd: 0x%04x\n", ntoh16(message->hdr.hrd));
    infof( "        pro: 0x%04x\n", ntoh16(message->hdr.pro));
    infof( "        hln: %u\n", message->hdr.hln);
    infof( "        pln: %u\n", message->hdr.pln);
    infof( "         op: 0x%04x (%s)\n", ntoh16(message->hdr.op), arp_opcode_ntoa(message->hdr.op));
    infof( "        sha: %s\n", ether_addr_ntop(message->sha, addr, sizeof(addr)));
    memcpy(&spa, message->spa, sizeof(spa));
    infof( "        spa: %s\n", ip_addr_ntop(spa, addr, sizeof(addr)));
    infof( "        tha: %s\n", ether_addr_ntop(message->tha, addr, sizeof(addr)));
    memcpy(&tpa, message->tpa, sizeof(tpa));
    infof( "        tpa: %s\n", ip_addr_ntop(tpa, addr, sizeof(addr)));
#ifdef HEXDUMP
    hexdump(stderr, data, len);
#endif
#endif
}

/*
 * ARP Cache
 *
 * NOTE: ARP Cache functions must be called after mutex locked
 */

static void
arp_pending_drop(struct arp_cache *cache)
{
    if (cache->pending) {
        memory_free(cache->pending);
        cache->pending = NULL;
    }
    cache->pending_len = 0;
    cache->iface = NULL;
    timerclear(&cache->request);
}

static struct arp_cache *
arp_cache_alloc(void)
{
    struct arp_cache *entry, *oldest = NULL;

    for (entry = caches; entry < tailof(caches); entry++) {
        if (entry->state == ARP_CACHE_STATE_FREE) {
            return entry;
        }
        if (!oldest || timercmp(&oldest->timestamp, &entry->timestamp, >)) {
            oldest = entry;
        }
    }
    if (oldest) {
        /* the evicted entry may still own a queued datagram */
        arp_pending_drop(oldest);
    }
    return oldest;
}

static struct arp_cache *
arp_cache_select(ip_addr_t pa)
{
    struct arp_cache *entry, *candidate = NULL;

    for (entry = caches; entry < tailof(caches); entry++) {
        if (entry->state != ARP_CACHE_STATE_FREE && entry->pa == pa) {
            if (entry->state == ARP_CACHE_STATE_RESOLVED ||
                entry->state == ARP_CACHE_STATE_STATIC) {
                /*
                 * Only a usable entry gets its lifetime refreshed. Touching
                 * the timestamp of an INCOMPLETE entry made it immortal, so
                 * arp_timer() could never reap an address that never replies.
                 */
                gettimeofday(&entry->timestamp, NULL);
                return entry;
            }
            if (!candidate) {
                candidate = entry;
            }
        }
    }
    return candidate;
}

static struct arp_cache *
arp_cache_update(ip_addr_t pa, const uint8_t *ha)
{
    struct arp_cache *cache;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[ETHER_ADDR_STR_LEN];

    cache = arp_cache_select(pa);
    if (!cache) {
        /* not found */
        return NULL;
    }
    cache->state = ARP_CACHE_STATE_RESOLVED;
    memcpy(cache->ha, ha, ETHER_ADDR_LEN);
    gettimeofday(&cache->timestamp, NULL);
    debugf("UPDATE: pa=%s, ha=%s", ip_addr_ntop(pa, addr1, sizeof(addr1)), ether_addr_ntop(ha, addr2, sizeof(addr2)));
    return cache;
}

static struct arp_cache *
arp_cache_insert(ip_addr_t pa, const uint8_t *ha)
{
    struct arp_cache *cache;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[ETHER_ADDR_STR_LEN];

    cache = arp_cache_select(pa);
    if (!cache) {
        cache = arp_cache_alloc();
    }
    if (!cache) {
        errorf("arp_cache_alloc() failure");
        return NULL;
    }
    cache->state = ARP_CACHE_STATE_RESOLVED;
    cache->pa = pa;
    memcpy(cache->ha, ha, ETHER_ADDR_LEN);
    gettimeofday(&cache->timestamp, NULL);
    debugf("INSERT: pa=%s, ha=%s", ip_addr_ntop(pa, addr1, sizeof(addr1)), ether_addr_ntop(ha, addr2, sizeof(addr2)));
    return cache;
}

static void
arp_cache_delete(struct arp_cache *cache)
{
    char addr1[IP_ADDR_STR_LEN];
    char addr2[ETHER_ADDR_STR_LEN];

    debugf("DELETE: pa=%s, ha=%s", ip_addr_ntop(cache->pa, addr1, sizeof(addr1)), ether_addr_ntop(cache->ha, addr2, sizeof(addr2)));
    arp_pending_drop(cache);
    cache->state = ARP_CACHE_STATE_FREE;
    cache->pa = 0;
    memset(cache->ha, 0, ETHER_ADDR_LEN);
    timerclear(&cache->timestamp);
}

static int
arp_request_due(struct arp_cache *cache)
{
    struct timeval now, diff;

    if (!cache->request.tv_sec && !cache->request.tv_usec) {
        return 1;
    }
    gettimeofday(&now, NULL);
    timersub(&now, &cache->request, &diff);
    if (diff.tv_sec > 0 || diff.tv_usec >= ARP_REQUEST_INTERVAL_US) {
        return 1;
    }
    return 0;
}

static int
arp_request(struct net_iface *iface, ip_addr_t tpa)
{
    struct arp_ether request;

    request.hdr.hrd = hton16(ARP_HRD_ETHER);
    request.hdr.pro = hton16(ARP_PRO_IP);
    request.hdr.hln = ETHER_ADDR_LEN;
    request.hdr.pln = IP_ADDR_LEN;
    request.hdr.op = hton16(ARP_OP_REQUEST);
    memcpy(request.sha, iface->dev->addr, ETHER_ADDR_LEN);
    memcpy(request.spa, &((struct ip_iface *)iface)->unicast, IP_ADDR_LEN);
    memset(request.tha, 0, ETHER_ADDR_LEN);
    memcpy(request.tpa, &tpa, IP_ADDR_LEN);
    debugf("dev=%s, opcode=%s(0x%04x), len=%zu", iface->dev->name, arp_opcode_ntoa(request.hdr.op), ntoh16(request.hdr.op), sizeof(request));
    arp_dump((uint8_t *)&request, sizeof(request));
    return net_device_output(iface->dev, ETHER_TYPE_ARP, (uint8_t *)&request, sizeof(request), iface->dev->broadcast);
}

static int
arp_reply(struct net_iface *iface, const uint8_t *tha, ip_addr_t tpa, const uint8_t *dst)
{
    struct arp_ether reply;

    reply.hdr.hrd = hton16(ARP_HRD_ETHER);
    reply.hdr.pro = hton16(ARP_PRO_IP);
    reply.hdr.hln = ETHER_ADDR_LEN;
    reply.hdr.pln = IP_ADDR_LEN;
    reply.hdr.op = hton16(ARP_OP_REPLY);
    memcpy(reply.sha, iface->dev->addr, ETHER_ADDR_LEN);
    memcpy(reply.spa, &((struct ip_iface *)iface)->unicast, IP_ADDR_LEN);
    memcpy(reply.tha, tha, ETHER_ADDR_LEN);
    memcpy(reply.tpa, &tpa, IP_ADDR_LEN);
    debugf("dev=%s, opcode=%s(0x%04x), len=%zu", iface->dev->name, arp_opcode_ntoa(reply.hdr.op), ntoh16(reply.hdr.op), sizeof(reply));
    arp_dump((uint8_t *)&reply, sizeof(reply));
    return net_device_output(iface->dev, ETHER_TYPE_ARP, (uint8_t *)&reply, sizeof(reply), dst);
}

static struct timeval arp_reply_window; /* window start for the reply limiter */
static int arp_reply_window_count;      /* replies admitted in current window */

static int
arp_reply_allowed(void)
{
    struct timeval now, diff;
    int allowed;

    mutex_lock(&mutex);
    gettimeofday(&now, NULL);
    timersub(&now, &arp_reply_window, &diff);
    if (diff.tv_sec > 0 || diff.tv_usec >= ARP_REPLY_WINDOW_US) {
        arp_reply_window = now;
        arp_reply_window_count = 0;
    }
    allowed = (arp_reply_window_count < ARP_REPLY_MAX_PER_WINDOW);
    if (allowed) {
        arp_reply_window_count++;
    } else {
        /*
         * Over the cap: this can only happen under an ARP flood, since
         * legitimate resolution stays far below the ceiling. Drop the
         * reply quietly; the requester retransmits and the next window
         * will serve it.
         */
        debugf("arp reply rate-limited");
    }
    mutex_unlock(&mutex);
    return allowed;
}

static void
arp_input(const uint8_t *data, size_t len, struct net_device *dev)
{
    struct arp_ether *msg;
    ip_addr_t spa, tpa;
    int merge = 0;
    struct net_iface *iface;
    struct arp_cache *cache;
    struct net_iface *pending_iface = NULL;
    uint8_t *pending = NULL;
    size_t pending_len = 0;
    uint8_t pending_ha[ETHER_ADDR_LEN] = {};

    if (len < sizeof(*msg)) {
        errorf("too short");
        return;
    }
    msg = (struct arp_ether *)data;
    if (ntoh16(msg->hdr.hrd) != ARP_HRD_ETHER || msg->hdr.hln != ETHER_ADDR_LEN) {
        errorf("unsupported hardware address");
        return;
    }
    if (ntoh16(msg->hdr.pro) != ARP_PRO_IP || msg->hdr.pln != IP_ADDR_LEN) {
        errorf("unsupported protocol address");
        return;
    }
    debugf("dev=%s, opcode=%s(0x%04x), len=%zu", dev->name, arp_opcode_ntoa(msg->hdr.op), ntoh16(msg->hdr.op), len);
    arp_dump(data, len);
    memcpy(&spa, msg->spa, sizeof(spa));
    memcpy(&tpa, msg->tpa, sizeof(tpa));
    mutex_lock(&mutex);
    cache = arp_cache_update(spa, msg->sha);
    if (cache) {
        /* updated */
        merge = 1;
        /* take ownership of the datagram that was waiting for this reply */
        pending = cache->pending;
        pending_len = cache->pending_len;
        pending_iface = cache->iface;
        cache->pending = NULL;
        cache->pending_len = 0;
        cache->iface = NULL;
        timerclear(&cache->request);
        memcpy(pending_ha, cache->ha, ETHER_ADDR_LEN);
    }
    mutex_unlock(&mutex);
    /* the flush must happen with the ARP mutex released */
    if (pending) {
        if (pending_iface) {
            net_device_output(pending_iface->dev, NET_PROTOCOL_TYPE_IP,
                    pending, pending_len, pending_ha);
        }
        memory_free(pending);
    }
    iface = net_device_get_iface(dev, NET_IFACE_FAMILY_IP);
    if (iface && ((struct ip_iface *)iface)->unicast == tpa) {
        if (!merge) {
            mutex_lock(&mutex);
            arp_cache_insert(spa, msg->sha);
            mutex_unlock(&mutex);
        }
        if (ntoh16(msg->hdr.op) == ARP_OP_REQUEST) {
            if (arp_reply_allowed()) {
                arp_reply(iface, msg->sha, spa, msg->sha);
            }
        }
    }
}

/*
 * Queue one datagram behind an unresolved neighbour entry.
 *
 * ip_output() must never wait for the ARP reply: it runs with the TCP/UDP
 * stack mutex held, on the shared IPC dispatch thread and on the single
 * protocol/timer thread. Blocking there stalled every socket in netd and the
 * DHCP timer as well. Instead the frame is parked here and transmitted from
 * arp_input() as soon as the reply lands.
 */
int
arp_pending_push(struct net_iface *iface, ip_addr_t pa, const uint8_t *data, size_t len)
{
    struct arp_cache *cache;
    uint8_t *buf;

    if (len == 0 || len > ARP_PENDING_MAX) {
        return -1;
    }
    buf = memory_alloc(len);
    if (!buf) {
        errorf("memory_alloc() failure");
        return -1;
    }
    memcpy(buf, data, len);

    mutex_lock(&mutex);
    cache = arp_cache_select(pa);
    if (!cache || cache->state != ARP_CACHE_STATE_INCOMPLETE) {
        /* raced with the reply, or the entry got evicted */
        mutex_unlock(&mutex);
        memory_free(buf);
        return -1;
    }
    /* keep only the newest datagram, like a one-deep neighbour queue */
    if (cache->pending) {
        memory_free(cache->pending);
    }
    cache->pending = buf;
    cache->pending_len = len;
    cache->iface = iface;
    mutex_unlock(&mutex);
    return 0;
}

int
arp_resolve(struct net_iface *iface, ip_addr_t pa, uint8_t *ha)
{
    struct arp_cache *cache;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[ETHER_ADDR_STR_LEN];

    if (iface->dev->type != NET_DEVICE_TYPE_ETHERNET) {
        debugf("unsupported hardware address type");
        return ARP_RESOLVE_ERROR;
    }
    if (iface->family != NET_IFACE_FAMILY_IP) {
        debugf("unsupported protocol address type");
        return ARP_RESOLVE_ERROR;
    }
    mutex_lock(&mutex);
    cache = arp_cache_select(pa);
    if (!cache) {
        cache = arp_cache_alloc();
        if (!cache) {
            mutex_unlock(&mutex);
            errorf("arp_cache_alloc() failure");
            return ARP_RESOLVE_ERROR;
        }
        cache->state = ARP_CACHE_STATE_INCOMPLETE;
        cache->pa = pa;
        cache->iface = iface;
        gettimeofday(&cache->timestamp, NULL);
        cache->request = cache->timestamp;
        arp_request(iface, pa);
        mutex_unlock(&mutex);
        debugf("cache not found, pa=%s", ip_addr_ntop(pa, addr1, sizeof(addr1)));
        return ARP_RESOLVE_INCOMPLETE;
    }
    if (cache->state == ARP_CACHE_STATE_INCOMPLETE) {
        cache->iface = iface;
        /*
         * Rate limited: this used to broadcast a request on every single call,
         * and ip_output() called it 300 times in a row, flooding the link with
         * ARP requests for one unreachable address.
         */
        if (arp_request_due(cache)) {
            gettimeofday(&cache->request, NULL);
            arp_request(iface, pa); /* just in case packet loss */
        }
        mutex_unlock(&mutex);
        return ARP_RESOLVE_INCOMPLETE;
    }
    memcpy(ha, cache->ha, ETHER_ADDR_LEN);
    mutex_unlock(&mutex);
    debugf("resolved, pa=%s, ha=%s",
        ip_addr_ntop(pa, addr1, sizeof(addr1)), ether_addr_ntop(ha, addr2, sizeof(addr2)));
    return ARP_RESOLVE_FOUND;
}

static void
arp_timer(void)
{
    struct arp_cache *entry;
    struct timeval now, diff;
    time_t timeout;

    mutex_lock(&mutex);
    gettimeofday(&now, NULL);
    for (entry = caches; entry < tailof(caches); entry++) {
        if (entry->state != ARP_CACHE_STATE_FREE && entry->state != ARP_CACHE_STATE_STATIC) {
            timeout = (entry->state == ARP_CACHE_STATE_INCOMPLETE) ?
                    ARP_INCOMPLETE_TIMEOUT : ARP_CACHE_TIMEOUT;
            timersub(&now, &entry->timestamp, &diff);
            if (diff.tv_sec > timeout) {
                arp_cache_delete(entry);
            }
        }
    }
    mutex_unlock(&mutex);
}

int
arp_init(void)
{
    struct timeval interval = {1, 0};

    if (net_protocol_register("ARP", NET_PROTOCOL_TYPE_ARP, arp_input) == -1) {
        errorf("net_protocol_register() failure");
        return -1;
    }
    if (net_timer_register("ARP Timer", interval, arp_timer) == -1) {
        errorf("net_timer_register() failure");
        return -1;
    }
    return 0;
}
