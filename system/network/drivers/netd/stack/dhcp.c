#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <ewoksys/klog.h>

#include "../platform.h"

#include "util.h"
#include "net.h"
#include "ether.h"
#include "sock.h"
#include "ip.h"
#include "udp.h"

#define DHCP_CHADDR_LEN                     16
#define DHCP_SNAME_LEN                      64
#define DHCP_FILE_LEN                       128

#define DHCP_BOOTREQUEST                    1
#define DHCP_BOOTREPLY                      2

#define DHCP_HARDWARE_TYPE_10_EHTHERNET     1

#define MESSAGE_TYPE_PAD                    0
#define MESSAGE_TYPE_REQ_SUBNET_MASK        1
#define MESSAGE_TYPE_ROUTER                 3
#define MESSAGE_TYPE_DNS                    6
#define MESSAGE_TYPE_DOMAIN_NAME            15
#define MESSAGE_TYPE_REQ_IP                 50
#define MESSAGE_TYPE_LEASE_TIME             51
#define MESSAGE_TYPE_DHCP                   53
#define MESSAGE_TYPE_SERVER_IP              54
#define MESSAGE_TYPE_PARAMETER_REQ_LIST     55
#define MESSAGE_TYPE_END                    255

#define DHCP_OPTION_DISCOVER                1
#define DHCP_OPTION_OFFER                   2
#define DHCP_OPTION_REQUEST                 3
#define DHCP_OPTION_DECLINE                 4
#define DHCP_OPTION_ACK                     5
#define DHCP_OPTION_NAK                     6

#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define DHCP_MAGIC_COOKIE   0x63825363
#define DHCP_FLAGS_BROADCAST 0x8000
#define DHCP_RETRY_INTERVAL  3
#define DHCP_DEFAULT_LEASE   3600

enum {
    DHCP_STATE_INIT = 0,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
};

struct ether_hdr {
    uint8_t dst[ETHER_ADDR_LEN];
    uint8_t src[ETHER_ADDR_LEN];
    uint16_t type;
};

struct ip_hdr {
    uint8_t vhl;
    uint8_t tos;
    uint16_t total;
    uint16_t id;
    uint16_t offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t sum;
    ip_addr_t src;
    ip_addr_t dst;
};

struct udp_hdr {
    uint16_t src;
    uint16_t dst;
    uint16_t len;
    uint16_t sum;
};

/*
 * http://www.tcpipguide.com/free/t_DHCPMessageFormat.htm
 */
typedef struct dhcp
{
    uint8_t    opcode;
    uint8_t    htype;
    uint8_t    hlen;
    uint8_t    hops;
    uint32_t   xid;
    uint16_t   secs;
    uint16_t   flags;
    uint32_t       ciaddr;
    uint32_t       yiaddr;
    uint32_t       siaddr;
    uint32_t       giaddr;
    uint8_t    chaddr[DHCP_CHADDR_LEN];
    char        bp_sname[DHCP_SNAME_LEN];
    char        bp_file[DHCP_FILE_LEN];
    uint32_t    magic_cookie;
    uint8_t    bp_options[0];
} dhcp_t;

typedef struct dhcp_client{
    struct  dhcp_client* next;
    struct net_device* dev;
    struct timeval  update;   
    uint32_t   xid;
    uint32_t   requested_ip;
    uint32_t   server_id;
    uint32_t   ip;
    uint32_t   netmask;
    uint32_t   gateway;
    uint32_t   dns1;
    uint32_t   dns2;
    int   validity; 
    int   state;

}dhcp_client_t;

static dhcp_client_t *dhcp_client_list = NULL; 

static dhcp_client_t *get_client(struct net_device *dev){
    dhcp_client_t *dhc = dhcp_client_list;
    while(dhc){
        if(dhc->dev == dev){
            break;
        }
        dhc = dhc->next;
    }
    return dhc;
}

/*
 * Return checksum for the given data.
 * Copied from FreeBSD
 */
static unsigned short
in_cksum(unsigned short *addr, int len)
{
    register int sum = 0;
    uint16_t answer = 0;
    register uint16_t *w = addr;
    register int nleft = len;
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
        *(uint8_t *)(&answer) = *(uint8_t *) w;
        sum += answer;
    }
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);             /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}

/*
 * Adds DHCP option to the bytestream
 */
static int
fill_dhcp_option(u_int8_t *packet, u_int8_t code, u_int8_t *data, u_int8_t len)
{
    packet[0] = code;
    packet[1] = len;
    memcpy(&packet[2], data, len);

    return len + (sizeof(u_int8_t) * 2);
}

/*
 * Fill DHCP options
 */
static int
fill_dhcp_discovery_options(dhcp_t *dhcp)
{
    int len = 0;
    u_int8_t parameter_req_list[] = {MESSAGE_TYPE_REQ_SUBNET_MASK, MESSAGE_TYPE_ROUTER, MESSAGE_TYPE_DNS, MESSAGE_TYPE_DOMAIN_NAME};
    u_int8_t option;

    option = DHCP_OPTION_DISCOVER;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_DHCP, &option, sizeof(option));
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_PARAMETER_REQ_LIST, (u_int8_t *)&parameter_req_list, sizeof(parameter_req_list));
    option = 0;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_END, &option, sizeof(option));

    return len;
}

static int
fill_dhcp_request_options(dhcp_t *dhcp, uint32_t requested_ip, uint32_t server_id)
{
    int len = 0;
    u_int8_t parameter_req_list[] = {MESSAGE_TYPE_REQ_SUBNET_MASK, MESSAGE_TYPE_ROUTER, MESSAGE_TYPE_DNS, MESSAGE_TYPE_DOMAIN_NAME};
    u_int8_t option;
    uint32_t req_ip;
    uint32_t srv_ip;

    option = DHCP_OPTION_REQUEST;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_DHCP, &option, sizeof(option));

    req_ip = requested_ip;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_REQ_IP, (u_int8_t *)&req_ip, sizeof(req_ip));

    srv_ip = server_id;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_SERVER_IP, (u_int8_t *)&srv_ip, sizeof(srv_ip));
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_PARAMETER_REQ_LIST, (u_int8_t *)&parameter_req_list, sizeof(parameter_req_list));

    option = 0;
    len += fill_dhcp_option(&dhcp->bp_options[len], MESSAGE_TYPE_END, &option, sizeof(option));

    return len;
}

static uint32_t
dhcp_generate_xid(struct net_device *dev)
{
    struct timeval now;
    uint32_t xid;
    int i;

    gettimeofday(&now, NULL);
    xid = (uint32_t)now.tv_sec ^ (uint32_t)now.tv_usec;
    for (i = 0; i < ETHER_ADDR_LEN; i++) {
        xid ^= ((uint32_t)dev->addr[i]) << ((i % sizeof(xid)) * 8);
        xid = (xid << 5) | (xid >> 27);
    }
    if (xid == 0) {
        xid = 1;
    }
    return xid;
}

/*
 * DHCP output - Just fills DHCP_BOOTREQUEST
 */
static void
dhcp_fill(struct net_device *dev, dhcp_t *dhcp, int *len, uint32_t xid)
{
    *len += sizeof(dhcp_t);
    memset(dhcp, 0, sizeof(dhcp_t));

    dhcp->opcode = DHCP_BOOTREQUEST;
    dhcp->htype = DHCP_HARDWARE_TYPE_10_EHTHERNET;
    dhcp->hlen = 6;
    dhcp->xid = hton32(xid);
    dhcp->flags = hton16(DHCP_FLAGS_BROADCAST);
    memcpy(dhcp->chaddr, dev->addr, 6);
    dhcp->magic_cookie = hton32(DHCP_MAGIC_COOKIE);
}

/*
 * UDP output - Fills appropriate bytes in UDP header
 */
static void
udp_fill(struct udp_hdr *udp_header, int *len)
{
    if (*len & 1)
        *len += 1;
    *len += sizeof(struct udp_hdr);

    udp_header->src= hton16(DHCP_CLIENT_PORT);
    udp_header->dst = hton16(DHCP_SERVER_PORT);
    udp_header->len = hton16(*len);
    udp_header->sum = 0;
}

/*
 * IP Output handler - Fills appropriate bytes in IP header
 */
static void
ip_fill(struct ip_hdr *ip_header, int *len)
{
    *len += sizeof(struct ip_hdr);

    ip_header->vhl = (IP_VERSION_IPV4 << 4) | (sizeof(struct ip_hdr) >> 2);
    ip_header->tos = 0x10;
    ip_header->total = hton16(*len);
    ip_header->id = hton16(0xffff);
    ip_header->offset = 0;
    ip_header->ttl = 16;
    ip_header->protocol = IPPROTO_UDP;
    ip_header->sum = 0;
    ip_header->src = 0;
    ip_header->dst = 0xFFFFFFFF;

    ip_header->sum = in_cksum((unsigned short *) ip_header, sizeof(struct ip_hdr));
}

static int
dhcp_send(struct net_device *dev, dhcp_client_t *dhc, int message_type)
{
    int len = 0;
    uint8_t *packet = malloc(1544);
    struct udp_hdr *udp_header;
    struct ip_hdr *ip_header;
    dhcp_t *dhcp;
    const uint8_t broadcast[] =  {0xff,0xff, 0xff, 0xff, 0xff, 0xff};
    int ret;

    if (packet == NULL) {
        return -1;
    }

    ip_header = (struct ip_hdr *)packet;
    udp_header = (struct udp_hdr *)(((char *)ip_header) + sizeof(struct ip_hdr));
    dhcp = (dhcp_t *)(((char *)udp_header) + sizeof(struct udp_hdr));

    if (message_type == DHCP_OPTION_DISCOVER) {
        len = fill_dhcp_discovery_options(dhcp);
    } else if (message_type == DHCP_OPTION_REQUEST) {
        len = fill_dhcp_request_options(dhcp, dhc->requested_ip, dhc->server_id);
    } else {
        free(packet);
        return -1;
    }

    dhcp_fill(dev, dhcp, &len, dhc->xid);
    udp_fill(udp_header, &len);
    ip_fill(ip_header, &len);

    ret = net_device_output(dev, ETHER_TYPE_IP, packet, len, broadcast);
    free(packet);
    return ret;
}

static void
dhcp_discovery(struct net_device *dev)
{
    dhcp_client_t *dhc = get_client(dev);

    if (dhc == NULL) {
        return;
    }

    dhc->xid = dhcp_generate_xid(dev);
    dhc->requested_ip = 0;
    dhc->server_id = 0;
    dhc->state = DHCP_STATE_SELECTING;
    gettimeofday(&dhc->update, NULL);
    dhcp_send(dev, dhc, DHCP_OPTION_DISCOVER);
}

static void
dhcp_request(struct net_device *dev, dhcp_client_t *dhc)
{
    if (dhc == NULL || dhc->requested_ip == 0 || dhc->server_id == 0) {
        return;
    }
    dhc->state = DHCP_STATE_REQUESTING;
    gettimeofday(&dhc->update, NULL);
    dhcp_send(dev, dhc, DHCP_OPTION_REQUEST);
}

static void
dhcp_reset(struct net_device *dev, dhcp_client_t *dhc)
{
    if (dhc == NULL) {
        return;
    }
    dhc->requested_ip = 0;
    dhc->server_id = 0;
    dhc->ip = 0;
    dhc->netmask = 0;
    dhc->gateway = 0;
    dhc->dns1 = 0;
    dhc->dns2 = 0;
    dhc->validity = DHCP_RETRY_INTERVAL;
    dhc->state = DHCP_STATE_INIT;
    dhcp_discovery(dev);
}

static void
dhcp_log_addr(const char *name, uint32_t addr)
{
    char buf[16];

    ip_addr_ntop(addr, buf, sizeof(buf));
    slog("%s: %s\n", name, buf);
}

static void dhcp_input(const uint8_t *data, size_t len, struct net_device *dev)
{
    struct ip_hdr *ip_packet;
    struct udp_hdr* udp_packet;
    dhcp_t* dhcp;
    uint16_t ip_hlen;
    uint16_t ip_total;
    uint16_t udp_len;
    int size;
    uint8_t *option;
    uint32_t cookie;
    uint8_t dhcp_type = 0;
    uint32_t server_id = 0;
    uint32_t requested_ip = 0;
    int lease_time = 0;

    //hexdump(stderr, data, len);
    dhcp_client_t *dhc =  get_client(dev);
    if(!dhc)
        return;

    if (len < sizeof(struct ip_hdr))
        return;

    ip_packet = (struct ip_hdr *)(data);
    if ((ip_packet->vhl >> 4) != 4)
        return;

    ip_hlen = (uint16_t)((ip_packet->vhl & 0x0f) << 2);
    if (ip_hlen < sizeof(struct ip_hdr) || len < ip_hlen + sizeof(struct udp_hdr))
        return;

    ip_total = ntoh16(ip_packet->total);
    if (ip_total < ip_hlen + sizeof(struct udp_hdr) || len < ip_total)
        return;

    /* Care only about UDP - since DHCP sits over UDP */
    if (ip_packet->protocol != IPPROTO_UDP)
        return;

    udp_packet = (struct udp_hdr *)((char *)ip_packet + ip_hlen);
    udp_len = ntoh16(udp_packet->len);
    if (udp_len < sizeof(struct udp_hdr) + sizeof(dhcp_t))
        return;
    if ((size_t)ip_hlen + udp_len > len)
        return;

    /* Check if there is a response from DHCP server by checking the source Port */
    if (ntoh16(udp_packet->src) != DHCP_SERVER_PORT)
        return;

    dhcp = ((dhcp_t *)((char *)udp_packet + sizeof(struct udp_hdr)));
    if (dhcp->opcode != DHCP_BOOTREPLY)
        return;
    if (dhcp->hlen != 6)
        return;
    if (memcmp(dhcp->chaddr, dev->addr, ETHER_ADDR_LEN) != 0)
        return;
    if (ntoh32(dhcp->xid) != dhc->xid)
        return;
    cookie = ntoh32(dhcp->magic_cookie);
    if (cookie != DHCP_MAGIC_COOKIE)
        return;

    /* Parse dhcp option*/
    option  = dhcp->bp_options;
    size = (int)udp_len - (int)sizeof(struct udp_hdr) - (int)sizeof(dhcp_t);

    while(size > 0){
        uint8_t code = option[0];
        uint8_t opt_len;

        if (code == MESSAGE_TYPE_PAD) {
            option++;
            size--;
            continue;
        }
        if (code == MESSAGE_TYPE_END)
            goto done;
        if (size < 2)
            return;

        opt_len = option[1];
        if (size < (int)opt_len + 2)
            return;
        switch(code){
            case MESSAGE_TYPE_DHCP:
                if (opt_len >= 1)
                    dhcp_type = option[2];
                break;
            case MESSAGE_TYPE_LEASE_TIME:
                if (opt_len >= 4)
                    lease_time = ntoh32(*(uint32_t*)(&option[2]));
                break; 
            case MESSAGE_TYPE_ROUTER:
                if (opt_len >= 4)
                    dhc->gateway = *(uint32_t*)(&option[2]);
                break;
            case MESSAGE_TYPE_REQ_SUBNET_MASK:
                if (opt_len >= 4)
                    dhc->netmask = *(uint32_t*)(&option[2]); 
                break;
            case MESSAGE_TYPE_DNS:
                if (opt_len >= 4)
                    dhc->dns1 = *(uint32_t*)(&option[2]); 
                break;
            case MESSAGE_TYPE_SERVER_IP:
                if (opt_len >= 4)
                    server_id = *(uint32_t*)(&option[2]);
                break;
            case MESSAGE_TYPE_REQ_IP:
                if (opt_len >= 4)
                    requested_ip = *(uint32_t*)(&option[2]);
                break;
        }
        option += opt_len + 2;
        size -= opt_len + 2;
    }
done:
    if (dhcp_type == DHCP_OPTION_NAK) {
        gettimeofday(&dhc->update, NULL);
        dhcp_reset(dev, dhc);
        return;
    }

    if (dhcp_type == DHCP_OPTION_OFFER) {
        if (dhc->state != DHCP_STATE_SELECTING)
            return;
        dhc->requested_ip = dhcp->yiaddr != 0 ? dhcp->yiaddr : requested_ip;
        dhc->server_id = server_id != 0 ? server_id : dhcp->siaddr;
        if (dhc->requested_ip == 0 || dhc->server_id == 0)
            return;
        gettimeofday(&dhc->update, NULL);
        dhcp_request(dev, dhc);
        return;
    }

    if (dhcp_type != DHCP_OPTION_ACK)
        return;
    if (dhc->state != DHCP_STATE_REQUESTING && dhc->state != DHCP_STATE_SELECTING)
        return;
    if (server_id != 0 && dhc->server_id != 0 && server_id != dhc->server_id)
        return;

    dhc->ip = dhcp->yiaddr != 0 ? dhcp->yiaddr : dhc->requested_ip;
    if (dhc->ip == 0)
        return;
    dhc->validity = lease_time > 0 ? lease_time : DHCP_DEFAULT_LEASE;
    dhc->state = DHCP_STATE_BOUND;
    gettimeofday(&dhc->update, NULL);
    dhcp_log_addr("DHCP IP", dhc->ip);
    dhcp_log_addr("DHCP GATEWAY", dhc->gateway);
    ip_iface_update(net_device_get_iface(dev, NET_IFACE_FAMILY_IP), dhc->ip, dhc->netmask, dhc->gateway);
}

static void
dhcp_timer(void)
{
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    dhcp_client_t *dhc = dhcp_client_list;
    while(dhc){
        timersub(&now, &dhc->update, &diff);
        if (dhc->state == DHCP_STATE_BOUND) {
            if (diff.tv_sec >= dhc->validity) {
                dhcp_reset(dhc->dev, dhc);
            }
        } else if (diff.tv_sec >= DHCP_RETRY_INTERVAL) {
            dhcp_discovery(dhc->dev);
        }
        dhc = dhc->next;
    }
}

int  
dhcp_run(struct net_device* dev){
    dhcp_client_t *dhc = malloc(sizeof(dhcp_client_t));
    if (dhc == NULL) {
        return -1;
    }

    memset(dhc, 0, sizeof(*dhc));
    dhc->dev = dev;
    dhc->validity = DHCP_RETRY_INTERVAL;
    dhc->state = DHCP_STATE_INIT;
    dhc->next = dhcp_client_list;
    dhcp_client_list = dhc;
    gettimeofday(&dhc->update, NULL);
    return 0;
}

int
dhcp_init(void)
{
    struct timeval interval = {1, 0};

    if (net_protocol_register("RAW", NET_PROTOCOL_TYPE_RAW, dhcp_input) == -1) {
        errorf("net_protocol_register() failure");
        return -1;
    }
    if (net_timer_register("DHCP Timer", interval, dhcp_timer) == -1) {
        errorf("net_timer_register() failure");
        return -1;
    }
    return 0;
}
