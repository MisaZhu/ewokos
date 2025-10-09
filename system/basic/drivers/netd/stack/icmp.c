#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "ip.h"
#include "icmp.h"
#include "../platform.h"

#define ICMP_BUFSIZ IP_PAYLOAD_SIZE_MAX

struct icmp_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t sum;
    uint32_t values;
};

struct icmp_echo {
    uint8_t type;
    uint8_t code;
    uint16_t sum;
    uint16_t id;
    uint16_t seq;
};

static char *
icmp_type_ntoa(uint8_t type) {
    switch (type) {
    case ICMP_TYPE_ECHOREPLY:
        return "EchoReply";
    case ICMP_TYPE_DEST_UNREACH:
        return "DestinationUnreachable";
    case ICMP_TYPE_SOURCE_QUENCH:
        return "SourceQuench";
    case ICMP_TYPE_REDIRECT:
        return "Redirect";
    case ICMP_TYPE_ECHO:
        return "Echo";
    case ICMP_TYPE_TIME_EXCEEDED:
        return "TimeExceeded";
    case ICMP_TYPE_PARAM_PROBLEM:
        return "ParameterProblem";
    case ICMP_TYPE_TIMESTAMP:
        return "Timestamp";
    case ICMP_TYPE_TIMESTAMPREPLY:
        return "TimestampReply";
    case ICMP_TYPE_INFO_REQUEST:
        return "InformationRequest";
    case ICMP_TYPE_INFO_REPLY:
        return "InformationReply";
    }
    return "Unknown";
}

static void
icmp_dump(const uint8_t *data, size_t len)
{
#ifdef NET_DEBUG
    struct icmp_hdr *hdr;
    struct icmp_echo *echo;

    hdr = (struct icmp_hdr *)data;
    slog( "       type: %u (%s)\n", hdr->type, icmp_type_ntoa(hdr->type));
    slog( "       code: %u\n", hdr->code);
    slog( "        sum: 0x%04x (0x%04x)\n", ntoh16(hdr->sum), ntoh16(cksum16((uint16_t *)data, len, -hdr->sum)));
    switch (hdr->type) {
    case ICMP_TYPE_ECHOREPLY:
    case ICMP_TYPE_ECHO:
        echo = (struct icmp_echo *)hdr;
        slog( "         id: %u\n", ntoh16(echo->id));
        slog( "        seq: %u\n", ntoh16(echo->seq));
        break;
    default:
        slog( "     values: 0x%08x\n", ntoh32(hdr->values));
        break;
    }
#ifdef HEXDUMP
    hexdump(stderr, data, len);
#endif
#endif
}

static void
icmp_input(const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst, struct ip_iface *iface)
{
    struct icmp_hdr *hdr;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[IP_ADDR_STR_LEN];
    char addr3[IP_ADDR_STR_LEN];

    if (len < sizeof(*hdr)) {
        errorf("too short");
        return;
    }
    hdr = (struct icmp_hdr *)data;
    if (cksum16((uint16_t *)data, len, 0) != 0) {
        errorf("checksum error, sum=0x%04x, verify=0x%04x", ntoh16(hdr->sum), ntoh16(cksum16((uint16_t *)data, len, -hdr->sum)));
        return;
    }
    debugf("%s => %s, type=%s(%u), len=%zu, iface=%s",
        ip_addr_ntop(src, addr1, sizeof(addr1)),
        ip_addr_ntop(dst, addr2, sizeof(addr2)),
        icmp_type_ntoa(hdr->type), hdr->type, len,
        ip_addr_ntop(iface->unicast, addr3, sizeof(addr3)));
    icmp_dump(data, len);
    switch (hdr->type) {
    case ICMP_TYPE_ECHO:
        if (dst != iface->unicast) {
            /* message addressed to broadcast address.              */
            /* responds with the address of the received interface. */
            dst = iface->unicast;
        }
        icmp_output(ICMP_TYPE_ECHOREPLY, hdr->code, hdr->values, (uint8_t *)(hdr + 1), len - sizeof(*hdr), dst, src);
        break;
    default:
        /* ignore */
        break;
    }
}

int
icmp_output(uint8_t type, uint8_t code, uint32_t values, const uint8_t *data, size_t len, ip_addr_t src, ip_addr_t dst)
{
    struct icmp_hdr *hdr;
    size_t msg_len;
    char addr1[IP_ADDR_STR_LEN];
    char addr2[IP_ADDR_STR_LEN];

    uint8_t *buf = malloc(ICMP_BUFSIZ);
    if(!buf)
        return -1;

    hdr = (struct icmp_hdr *)buf;
    hdr->type = type;
    hdr->code = code;
    hdr->sum = 0;
    hdr->values = values;
    memcpy(hdr + 1, data, len);
    msg_len = sizeof(*hdr) + len;
    hdr->sum = cksum16((uint16_t *)hdr, msg_len, 0);
    debugf("%s => %s, type=%s(%u), len=%zu",
        ip_addr_ntop(src, addr1, sizeof(addr1)),
        ip_addr_ntop(dst, addr2, sizeof(addr2)),
        icmp_type_ntoa(hdr->type), hdr->type, msg_len);
    icmp_dump((uint8_t *)hdr, msg_len);

    int ret = ip_output(IP_PROTOCOL_ICMP, (uint8_t *)hdr, msg_len, src, dst);
    TRACE();
    free(buf);
    return ret;
}

int
icmp_init(void)
{
    if (ip_protocol_register("ICMP", IP_PROTOCOL_ICMP, icmp_input) == -1) {
        errorf("ip_protocol_register() failure");
        return -1;
    }
    return 0;
}
