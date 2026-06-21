#include "ntpc/ntpc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


// NTP timestamps start in 1900, while UNIX timestamps start in 1970.
// This is the number of seconds between the two epochs.
#define NTP_UNIX_OFFSET 2208988800UL
#define NTP_RECV_TIMEOUT_SEC 5

#ifdef __cplusplus 
extern "C" {
#endif

// NTP packet layout
struct ntp_packet {
    uint8_t li_vn_mode;      // Top 2 bits: leap indicator, middle 3: version, low 3: mode
    uint8_t stratum;         // Server stratum
    uint8_t poll;            // Poll interval
    uint8_t precision;       // Clock precision
    uint32_t root_delay;     // Root delay
    uint32_t root_dispersion;// Root dispersion
    uint32_t reference_id;   // Reference identifier
    uint32_t ref_ts_sec;     // Reference timestamp seconds
    uint32_t ref_ts_frac;    // Reference timestamp fraction
    uint32_t orig_ts_sec;    // Originate timestamp seconds
    uint32_t orig_ts_frac;   // Originate timestamp fraction
    uint32_t recv_ts_sec;    // Receive timestamp seconds
    uint32_t recv_ts_frac;   // Receive timestamp fraction
    uint32_t tx_ts_sec;      // Transmit timestamp seconds, the field we care about most
    uint32_t tx_ts_frac;     // Transmit timestamp fraction
} __attribute__((packed));

time_t ntpc_get_time(const char* server_ip, uint16_t port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct ntp_packet packet;
    const char *ntp_server_ip = server_ip;
    if(ntp_server_ip == NULL || strlen(ntp_server_ip) == 0)
        ntp_server_ip = DEFAULT_NTP_SERVER;
    
    uint16_t ntp_port = port;
    if(ntp_port == 0)
        ntp_port = DEFAULT_NTP_PORT;

    ssize_t bytes_received;

    // Create a UDP socket.
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return 0;
    }

    // NTP is a short request; keep the timeout short to avoid prolonged UDP wait polling.
    struct timeval timeout;
    timeout.tv_sec = NTP_RECV_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return 0;
    }

    // Fill in the server address.
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ntp_port);

    // Prefer parsing as an IPv4 literal, then fall back to hostname resolution.
    if (inet_pton(AF_INET, ntp_server_ip, &(server_addr.sin_addr)) <= 0) {
        struct hostent* he = gethostbyname(ntp_server_ip);
        if (he == NULL || he->h_addr_list == NULL || he->h_addr_list[0] == NULL) {
            close(sockfd);
            return 0;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Initialize the NTP request packet.
    memset(&packet, 0, sizeof(packet));
    // Use NTPv4 in client mode.
    packet.li_vn_mode = 0x1b; // 0b00 100 11 -> li=0, vn=4, mode=3 (client)

    // Send the NTP request.
    int res = sendto(sockfd, &packet, sizeof(packet), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) {
        close(sockfd);
        return 0;
    }
    // Receive the NTP response.
    socklen_t addr_len = sizeof(server_addr);
    bytes_received = recvfrom(sockfd, &packet, sizeof(packet), 0, 
                             (struct sockaddr *)&server_addr, &addr_len);
    if (bytes_received < 0) {
        close(sockfd);
        return 0;
    }
    // Close the socket.
    close(sockfd);

    // Convert the NTP timestamp to a UNIX timestamp.
    // NTP timestamps are big-endian, so convert them to host byte order first.
    return ntohl(packet.tx_ts_sec) - NTP_UNIX_OFFSET;
}

#ifdef __cplusplus 
}
#endif
