#ifndef _ARPA_INET_H
#define _ARPA_INET_H    1

#include <netinet/in.h>

typedef __socklen_t socklen_t;


extern in_addr_t inet_addr (const char *__cp) ;
extern in_addr_t inet_lnaof (struct in_addr __in) ;
extern struct in_addr inet_makeaddr (in_addr_t __net, in_addr_t __host);
extern in_addr_t inet_netof (struct in_addr __in) ;
extern in_addr_t inet_network (const char *__cp) ;
extern char *inet_ntoa (struct in_addr __in) ;
extern int inet_pton (int __af, const char *__restrict __cp,
              void *__restrict __buf) ;
extern const char *inet_ntop (int __af, const void *__restrict __cp,
                  char *__restrict __buf, socklen_t __len);
extern int inet_aton (const char *__cp, struct in_addr *__inp) ;
extern char *inet_net_ntop (int __af, const void *__cp, int __bits,
                char *__buf, size_t __len) ;
extern int inet_net_pton (int __af, const char *__cp,
              void *__buf, size_t __len) ;

extern unsigned int inet_nsap_addr (const char *__cp,
                    unsigned char *__buf, int __len) ;
extern char *inet_nsap_ntoa (int __len, const unsigned char *__cp,
                 char *__buf) ;

#endif
