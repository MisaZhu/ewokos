#ifndef NTPC_H
#define NTPC_H

#include <time.h>

// Default port used by NTP servers.
#define DEFAULT_NTP_PORT 123
// Hardcoded default NTP server address. Replace it if needed.
#define DEFAULT_NTP_SERVER "203.107.6.88"  // National Time Service Center NTP server

#ifdef __cplusplus 
extern "C" {
#endif

time_t ntpc_get_time(const char* server_ip, uint16_t port);

#ifdef __cplusplus 
}
#endif

#endif
