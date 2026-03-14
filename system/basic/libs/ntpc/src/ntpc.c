#include "ntpc/ntpc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


// NTP时间戳从1900年开始，而UNIX时间戳从1970年开始，相差70年的秒数
#define NTP_UNIX_OFFSET 2208988800UL


#ifdef __cplusplus 
extern "C" {
#endif

// NTP数据包结构
struct ntp_packet {
    uint8_t li_vn_mode;      // 前3位：leap indicator, 中间3位：version, 后2位：mode
    uint8_t stratum;         // 服务器层级
    uint8_t poll;            // 轮询间隔
    uint8_t precision;       // 精度
    uint32_t root_delay;     // 根延迟
    uint32_t root_dispersion;// 根离散度
    uint32_t reference_id;   // 参考标识符
    uint32_t ref_ts_sec;     // 参考时间戳秒部分
    uint32_t ref_ts_frac;    // 参考时间戳分数部分
    uint32_t orig_ts_sec;    // 原始时间戳秒部分
    uint32_t orig_ts_frac;   // 原始时间戳分数部分
    uint32_t recv_ts_sec;    // 接收时间戳秒部分
    uint32_t recv_ts_frac;   // 接收时间戳分数部分
    uint32_t tx_ts_sec;      // 发送时间戳秒部分（这是我们主要关心的字段）
    uint32_t tx_ts_frac;     // 发送时间戳分数部分
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

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return 0;
    }

    // 设置2秒超时
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return 0;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ntp_port);
    
    // 直接使用IP地址，不进行域名解析
    if (inet_pton(AF_INET, ntp_server_ip, &(server_addr.sin_addr)) <= 0) {
        close(sockfd);
        return 0;
    }

    // 初始化NTP数据包
    memset(&packet, 0, sizeof(packet));
    // 设置协议版本为4，客户端模式
    packet.li_vn_mode = 0x1b; // 0b00 100 11 -> li=0, vn=4, mode=3 (client)

    // 发送NTP请求
    int res = sendto(sockfd, &packet, sizeof(packet), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) {
        close(sockfd);
        return 0;
    }
    // 接收NTP响应
    socklen_t addr_len = sizeof(server_addr);
    bytes_received = recvfrom(sockfd, &packet, sizeof(packet), 0, 
                             (struct sockaddr *)&server_addr, &addr_len);
    if (bytes_received < 0) {
        close(sockfd);
        return 0;
    }
    // 关闭套接字
    close(sockfd);

    // 转换NTP时间戳到UNIX时间戳
    // NTP时间戳是大端格式，需要转换为本机字节序
    return ntohl(packet.tx_ts_sec) - NTP_UNIX_OFFSET;
}

#ifdef __cplusplus 
}
#endif