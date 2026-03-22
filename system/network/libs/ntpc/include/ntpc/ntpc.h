#ifndef NTPC_H
#define NTPC_H

#include <time.h>

// NTP服务器默认使用的端口号
#define DEFAULT_NTP_PORT 123
// 硬编码一个NTP服务器IP地址（可以根据需要更换）
#define DEFAULT_NTP_SERVER "202.120.2.101"  // 中国国家授时中心NTP服务器

#ifdef __cplusplus 
extern "C" {
#endif

time_t ntpc_get_time(const char* server_ip, uint16_t port);

#ifdef __cplusplus 
}
#endif

#endif