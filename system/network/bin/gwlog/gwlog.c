/* Diagnostic probe: ping the gateway and a wired peer once per second and
 * append timestamped results to /etc/gw-ping.log, so a wireless stall can be
 * correlated with the driver's persistent diag after the fact. */
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ewoksys/proc.h>

typedef struct icmp_hdr {
    uint8_t type, code;
    uint16_t checksum, id, seq;
} icmp_hdr_t;

static uint16_t checksum(void *b, int len) {
    uint16_t *buf = b;
    uint32_t sum = 0;
    while (len > 1) { sum += *buf++; len -= 2; }
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    return (uint16_t)~sum;
}

/* The rootfs only materializes file content on close (the wlan driver's diag
 * uses the same open/write/close-per-flush pattern), so keep the history in
 * memory and rewrite the whole file on every line. */
#define LOG_BYTES (128u * 1024)
static char ring[LOG_BYTES];
static unsigned ring_len;

static void logline(const char *fmt, ...) {
    char line[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    printf("%s", line);
    if ((unsigned)n >= LOG_BYTES) return;
    if (ring_len + n > LOG_BYTES - 1) {
        unsigned drop = ring_len + n - (LOG_BYTES - 1);
        if (drop > ring_len) drop = ring_len;
        while (drop < ring_len && ring[drop] != '\n') drop++;
        if (drop < ring_len) drop++;
        memmove(ring, ring + drop, ring_len - drop);
        ring_len -= drop;
    }
    memcpy(ring + ring_len, line, n);
    ring_len += n;
    ring[ring_len] = 0;
    int fd = open("/etc/gw-ping.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) return;
    write(fd, ring, (int)ring_len);
    close(fd);
}

int main(int argc, char *argv[]) {
    const char *targets[2] = {"192.168.5.1", "192.168.5.43"};
    if (argc >= 2) targets[0] = argv[1];
    if (argc >= 3) targets[1] = argv[2];

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) { printf("socket failed\n"); return -1; }
    struct timeval timeout = {1, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    unsigned seq = 0;
    uint16_t id = getpid() & 0xFFFF;
    logline("START pid=%d targets=%s,%s\n", (int)getpid(), targets[0], targets[1]);
    while (1) {
        for (int t = 0; t < 2; t++) {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            inet_pton(AF_INET, targets[t], &addr.sin_addr);

            icmp_hdr_t pkt;
            memset(&pkt, 0, sizeof(pkt));
            pkt.type = 8;
            pkt.id = id;
            pkt.seq = htons(seq & 0xFFFF);
            pkt.checksum = checksum(&pkt, sizeof(pkt));

            struct timeval send_time, recv_time;
            gettimeofday(&send_time, NULL);
            long stamp = send_time.tv_sec;
            if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                logline("%ld %s seq=%u SENDFAIL\n", stamp, targets[t], seq);
                continue;
            }
            char buf[1024];
            struct sockaddr_in from;
            socklen_t fromlen;
            int n = -1;
            /* Deadline-bounded match: the Mac's own ping stream keeps feeding
             * this raw socket, so a per-recv timeout alone never fires. */
            while (1) {
                struct timeval now;
                gettimeofday(&now, NULL);
                long elapsed_us = (now.tv_sec - send_time.tv_sec) * 1000000L +
                                  (now.tv_usec - send_time.tv_usec);
                if (elapsed_us >= 1000000) break;
                struct timeval left = {0, 1000000 - elapsed_us};
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &left, sizeof(left));
                fromlen = sizeof(from);
                n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
                if (n < 0) break;
                int ihl = (buf[0] & 15) * 4;
                icmp_hdr_t *r = (icmp_hdr_t *)(buf + ihl);
                if (n >= ihl + 8 && r->type == 0 && r->id == id &&
                    ntohs(r->seq) == (seq & 0xFFFF) &&
                    from.sin_addr.s_un.s_addr == addr.sin_addr.s_un.s_addr) break;
                n = -1;
            }
            gettimeofday(&recv_time, NULL);
            long rtt_us = (recv_time.tv_sec - send_time.tv_sec) * 1000000L +
                          (recv_time.tv_usec - send_time.tv_usec);
            if (n < 0) {
                logline("%ld %s seq=%u TIMEOUT\n", stamp, targets[t], seq);
            } else {
                logline("%ld %s seq=%u OK rtt=%ld.%ldms\n", stamp, targets[t], seq,
                        rtt_us / 1000, (rtt_us % 1000) / 100);
            }
        }
        seq++;
        usleep(1000000);
    }
    return 0;
}