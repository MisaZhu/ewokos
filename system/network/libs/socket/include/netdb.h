/*
 *  netdb.h - shim for EwokOS (host lookup lives in sys/socket.h)
 */

#ifndef NETDB_H
#define NETDB_H

#include <sys/socket.h>

#define AI_PASSIVE     0x0001
#define AI_CANONNAME   0x0002
#define AI_NUMERICHOST 0x0004
#define AI_NUMERICSERV 0x0008

#define NI_NUMERICHOST 0x0001
#define NI_NUMERICSERV 0x0002
#define NI_MAXHOST 1025
#define NI_MAXSERV 32

#define EAI_BADFLAGS  -1
#define EAI_NONAME    -2
#define EAI_AGAIN     -3
#define EAI_FAIL      -4
#define EAI_FAMILY    -6
#define EAI_SOCKTYPE  -7
#define EAI_SERVICE   -8
#define EAI_MEMORY    -10
#define EAI_OVERFLOW  -12

#endif /* NETDB_H */
