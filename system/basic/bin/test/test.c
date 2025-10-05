#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// NTP服务器默认使用的端口号
#define NTP_PORT 123

// 硬编码一个NTP服务器IP地址（可以根据需要更换）
#define DEFAULT_NTP_SERVER "210.72.145.44"  // 中国国家授时中心NTP服务器

// NTP时间戳从1900年开始，而UNIX时间戳从1970年开始，相差70年的秒数
#define NTP_UNIX_OFFSET 2208988800UL

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

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct ntp_packet packet;
    char *ntp_server_ip = DEFAULT_NTP_SERVER;
    ssize_t bytes_received;
    time_t current_time;
    struct tm *time_info;
    char time_string[64];

    // 如果用户提供了NTP服务器IP，则使用它
    if (argc > 1) {
        ntp_server_ip = argv[1];
    }

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        printf("Failed to create socket\n");
        return 1;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);
    
    // 直接使用IP地址，不进行域名解析
    if (inet_pton(AF_INET, ntp_server_ip, &(server_addr.sin_addr)) <= 0) {
        printf("Invalid NTP server IP address: %s\n", ntp_server_ip);
        close(sockfd);
        return 1;
    }

    // 初始化NTP数据包
    memset(&packet, 0, sizeof(packet));
    // 设置协议版本为4，客户端模式
    packet.li_vn_mode = 0x1b; // 0b00 100 11 -> li=0, vn=4, mode=3 (client)

    // 发送NTP请求
    int res = sendto(sockfd, &packet, sizeof(packet), 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(res < 0) {
        printf("Failed to send NTP request\n");
        close(sockfd);
        return 1;
    }
    printf("send NTP ok :%d, recving\n", res);

    // 接收NTP响应
    socklen_t addr_len = sizeof(server_addr);
    bytes_received = recvfrom(sockfd, &packet, sizeof(packet), 0, 
                             (struct sockaddr *)&server_addr, &addr_len);
    if (bytes_received < 0) {
        printf("Failed to receive NTP response\n");
        close(sockfd);
        return 1;
    }
    printf("recv NTP ok :%d\n", bytes_received);

    // 关闭套接字
    close(sockfd);

    // 转换NTP时间戳到UNIX时间戳
    // NTP时间戳是大端格式，需要转换为本机字节序
    current_time = ntohl(packet.tx_ts_sec) - NTP_UNIX_OFFSET;

    // 格式化并显示时间
    time_info = localtime(&current_time);
    if (time_info == NULL) {
        printf("Failed to convert NTP timestamp to local time\n");
        return 1;
    }

    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
    printf("%s : %s\n", ntp_server_ip, time_string);

    return 0;
}
