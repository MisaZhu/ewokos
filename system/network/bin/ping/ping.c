#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ewoksys/proc.h>

// ICMP header structure
typedef struct icmp_hdr {
    uint8_t type;          // ICMP message type
    uint8_t code;          // ICMP message code
    uint16_t checksum;     // ICMP message checksum
    uint16_t id;           // ICMP message ID
    uint16_t seq;          // ICMP message sequence number
    // Data follows
} icmp_hdr_t;

// ICMP message types
#define ICMP_ECHO_REQ 8
#define ICMP_ECHO_REPLY 0

// Calculate checksum
uint16_t checksum(void *b, int len) {
    uint16_t *buf = b;
    uint32_t sum = 0;
    uint16_t result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ping <ip_address>\n");
        return -1;
    }

    const char *ip_addr = argv[1];
    int sockfd;
    struct sockaddr_in addr;
    icmp_hdr_t icmp_pkt;
    char recv_buf[1024];
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);
    int seq = 0;

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("Failed to create socket\n");
        return -1;
    }

    // Set timeout for recvfrom
    struct timeval timeout;
    timeout.tv_sec = 1; // 1 second
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("Failed to set socket timeout\n");
        close(sockfd);
        return -1;
    }

    // Set address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    char resolved_ip[INET_ADDRSTRLEN];
    
    // Try to parse as IP address first
    if (inet_pton(AF_INET, ip_addr, &addr.sin_addr) <= 0) {
        // If not IP address, try to resolve as domain name
        struct hostent *host = gethostbyname(ip_addr);
        if (host == NULL) {
            printf("Invalid IP address or domain name\n");
            close(sockfd);
            return -1;
        }
        memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);
        inet_ntop(AF_INET, &addr.sin_addr, resolved_ip, INET_ADDRSTRLEN);
        printf("PING %s (%s)\n", ip_addr, resolved_ip);
    } else {
        strcpy(resolved_ip, ip_addr);
        printf("PING %s\n", ip_addr);
    }

    while (seq < 4) {
        // Construct ICMP packet
        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.type = ICMP_ECHO_REQ;
        icmp_pkt.code = 0;
        icmp_pkt.id = getpid() & 0xFFFF;
        icmp_pkt.seq = htons(seq);
        seq++;
        icmp_pkt.checksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

        // Record send time
        struct timeval send_time, recv_time;
        gettimeofday(&send_time, NULL);

        // Send ICMP packet
        if (sendto(sockfd, &icmp_pkt, sizeof(icmp_pkt), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            printf("Failed to send ICMP packet\n");
            continue;
        }

        // Receive response
        int n = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&recv_addr, &addr_len);
        if (n < 0) {
            printf("Request timeout\n");
            proc_usleep(1000000); // 1 second
            continue;
        }

        // Record receive time
        gettimeofday(&recv_time, NULL);

        // Calculate RTT in milliseconds
        long rtt_sec = recv_time.tv_sec - send_time.tv_sec;
        long rtt_usec = recv_time.tv_usec - send_time.tv_usec;
        if (rtt_usec < 0) {
            rtt_sec--;
            rtt_usec += 1000000;
        }
        double rtt_ms = rtt_sec * 1000.0 + rtt_usec / 1000.0;

        // Check if it's an ICMP echo reply
        struct ip *ip_hdr = (struct ip *)recv_buf;
        int ip_len = ip_hdr->ip_hl * 4;
        icmp_hdr_t *recv_icmp = (icmp_hdr_t *)(recv_buf + ip_len);

        if (recv_icmp->type == ICMP_ECHO_REPLY && recv_icmp->id == (getpid() & 0xFFFF)) {
            printf("%d bytes from %s: icmp_seq=%d time=%.1f ms\n", n - ip_len, resolved_ip, ntohs(recv_icmp->seq), rtt_ms);
        }

        proc_usleep(1000000); // 1 second
    }

    close(sockfd);
    return 0;
}
