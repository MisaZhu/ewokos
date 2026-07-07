#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "util.h"
#include "net.h"
#include "ether.h"

struct ether_hdr {
    uint8_t dst[ETHER_ADDR_LEN];
    uint8_t src[ETHER_ADDR_LEN];
    uint16_t type;
};

const uint8_t ETHER_ADDR_ANY[ETHER_ADDR_LEN] = {"\x00\x00\x00\x00\x00\x00"};
const uint8_t ETHER_ADDR_BROADCAST[ETHER_ADDR_LEN] = {"\xff\xff\xff\xff\xff\xff"};

int
ether_addr_pton(const char *p, uint8_t *n)
{
    int index;
    char *ep;
    long val;

    if (!p || !n) {
        return -1;
    }
    for (index = 0; index < ETHER_ADDR_LEN; index++) {
        val = strtoul(p, &ep, 16);
        if (ep == p || val < 0 || val > 0xff || (index < ETHER_ADDR_LEN - 1 && *ep != ':')) {
            break;
        }
        n[index] = (uint8_t)val;
        p = ep + 1;
    }
    if (index != ETHER_ADDR_LEN || *ep != '\0') {
        return -1;
    }
    return  0;
}

char *
ether_addr_ntop(const uint8_t *n, char *p, size_t size)
{
    if (!n || !p) {
        return NULL;
    }
    sprintf(p, "%02x:%02x:%02x:%02x:%02x:%02x", n[0], n[1], n[2], n[3], n[4], n[5]);
    return p;
}

static void
ether_dump(const uint8_t *frame, size_t flen)
{
#ifdef NET_DEBUG
    static const char *
    ether_type_ntoa(uint16_t type)
    {
        switch (ntoh16(type)) {
        case ETHER_TYPE_IP:
            return "IP";
        case ETHER_TYPE_ARP:
            return "ARP";
        case ETHER_TYPE_IPV6:
            return "IPv6";
        }
        return "UNKNOWN";
    }
    struct ether_hdr *hdr;
    char addr[ETHER_ADDR_STR_LEN];

    hdr = (struct ether_hdr *)frame;
    slog("        src: %s\n", ether_addr_ntop(hdr->src, addr, sizeof(addr)));
    slog("        dst: %s\n", ether_addr_ntop(hdr->dst, addr, sizeof(addr)));
    slog("       type: 0x%04x (%s)\n", ntoh16(hdr->type), ether_type_ntoa(hdr->type));
#ifdef HEXDUMP
    hexdump(stderr, frame, flen);
#endif
#endif
}

int
ether_transmit_helper(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst, ssize_t (*callback)(struct net_device *dev, const uint8_t *data, size_t len))
{
    uint8_t frame[ETHER_FRAME_SIZE_MAX] = {};
    struct ether_hdr *hdr;
    size_t flen, pad = 0;

    hdr = (struct ether_hdr *)frame;
    memcpy(hdr->dst, dst, ETHER_ADDR_LEN);
    memcpy(hdr->src, dev->addr, ETHER_ADDR_LEN);
    hdr->type = hton16(type);
    memcpy(hdr + 1, data, len);
    if (len < ETHER_PAYLOAD_SIZE_MIN) {
        pad = ETHER_PAYLOAD_SIZE_MIN - len;
    }
    flen = sizeof(*hdr) + len + pad;
    debugf("dev=%s, type=%s(0x%04x), len=%zu", dev->name, ether_type_ntoa(hdr->type), type, flen);
    ether_dump(frame, flen);
    return callback(dev, frame, flen) == (ssize_t)flen ? 0 : -1;
}

/*
 * Return value:
 *   -1 : no frame available (device queue drained)
 *    0 : frame drained and either dropped (not for us / unsupported type) OR
 *        a broadcast processed by the stack; caller keeps draining but must NOT
 *        treat this as "session activity" (i.e. must not force the busy cadence)
 *    1 : a unicast frame addressed to our MAC was delivered (real session
 *        activity that warrants the fast polling cadence)
 */
int
ether_poll_helper(struct net_device *dev, ssize_t (*callback)(struct net_device *dev, uint8_t *buf, size_t size))
{
    uint8_t frame[ETHER_FRAME_SIZE_MAX];
    ssize_t flen;
    struct ether_hdr *hdr;
    uint16_t type;
    int unicast_to_us;

    flen = callback(dev, frame, sizeof(frame));
    if (flen < (ssize_t)sizeof(*hdr)) {
        //errorf("input data is too short");
        return -1;
    }
    hdr = (struct ether_hdr *)frame;
    type = ntoh16(hdr->type);
    unicast_to_us = (memcmp(dev->addr, hdr->dst, ETHER_ADDR_LEN) == 0);
    if (!unicast_to_us) {
        if (memcmp(ETHER_ADDR_BROADCAST, hdr->dst, ETHER_ADDR_LEN) != 0) {
            /*
             * Frames for other hosts (foreign unicast) and multicast are
             * drained from the device queue but dropped. Returning -1 here
             * would make ether_tap_isr() think the queue is empty and leave
             * the remaining packets stuck in /dev/wl0.
             */
            return 0;
        }
    }
    if (type != ETHER_TYPE_IP && type != ETHER_TYPE_ARP) {
        /*
         * netd only implements IPv4/ARP on this path. Ignore other EtherTypes
         * after draining them from /dev/wl0 so unsupported traffic cannot
         * amplify load in upper layers.
         */
        return 0;
    }
    infof("dev=%s, type=%s(0x%04x), len=%zu\n", dev->name, ether_type_ntoa(hdr->type), type, flen);
    ether_dump(frame, flen);
    net_input_handler(type, (uint8_t *)(hdr + 1), flen - sizeof(*hdr), dev);
    /*
     * Broadcast frames (ARP who-has, DHCP, limited broadcast) are still
     * processed above, but ambient broadcast traffic must not pin netd to the
     * fast busy cadence or the idle CPU load fluctuates with wire noise. Only
     * unicast frames addressed to us represent an active session.
     */
    return unicast_to_us ? 1 : 0;
}

void
ether_setup_helper(struct net_device *dev)
{
    dev->type = NET_DEVICE_TYPE_ETHERNET;
    dev->mtu = ETHER_PAYLOAD_SIZE_MAX;
    dev->flags = (NET_DEVICE_FLAG_BROADCAST | NET_DEVICE_FLAG_NEED_ARP);
    dev->hlen = ETHER_HDR_SIZE;
    dev->alen = ETHER_ADDR_LEN;
    memcpy(dev->broadcast, ETHER_ADDR_BROADCAST, ETHER_ADDR_LEN);
    memcpy(dev->addr, ETHER_ADDR_ANY, ETHER_ADDR_LEN);
}
