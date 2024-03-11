#ifndef	_NETINET_IN_H
#define	_NETINET_IN_H	1

#include <stdint.h>

typedef uint32_t in_addr_t;
typedef uint32_t socklen_t;

typedef struct in_addr {
  union {
    struct {
      uint8_t s_b1;
      uint8_t s_b2;
      uint8_t s_b3;
      uint8_t s_b4;
    } S_un_b;
    struct {
      uint16_t s_w1;
      uint16_t s_w2;
    } S_un_w;
    uint32_t S_addr;
  } S_un;
} IN_ADDR, *PIN_ADDR, *LPIN_ADDR;

struct sockaddr_in {
	uint8_t    sin_len;
	uint8_t sin_family;
	uint16_t   sin_port;
	struct	    in_addr sin_addr;
	int8_t	    sin_zero[8];
};


enum
  {
    IPPROTO_IP = 0,	   /* Dummy protocol for TCP.  */
    IPPROTO_ICMP = 1,	   /* Internet Control Message Protocol.  */
    IPPROTO_IGMP = 2,	   /* Internet Group Management Protocol. */
    IPPROTO_IPIP = 4,	   /* IPIP tunnels (older KA9Q tunnels use 94).  */
    IPPROTO_TCP = 6,	   /* Transmission Control Protocol.  */
    IPPROTO_EGP = 8,	   /* Exterior Gateway Protocol.  */
    IPPROTO_PUP = 12,	   /* PUP protocol.  */
    IPPROTO_UDP = 17,	   /* User Datagram Protocol.  */
    IPPROTO_IDP = 22,	   /* XNS IDP protocol.  */
    IPPROTO_TP = 29,	   /* SO Transport Protocol Class 4.  */
    IPPROTO_DCCP = 33,	   /* Datagram Congestion Control Protocol.  */
    IPPROTO_IPV6 = 41,     /* IPv6 header.  */
    IPPROTO_RSVP = 46,	   /* Reservation Protocol.  */
    IPPROTO_GRE = 47,	   /* General Routing Encapsulation.  */
    IPPROTO_ESP = 50,      /* encapsulating security payload.  */
    IPPROTO_AH = 51,       /* authentication header.  */
    IPPROTO_MTP = 92,	   /* Multicast Transport Protocol.  */
    IPPROTO_BEETPH = 94,   /* IP option pseudo header for BEET.  */
    IPPROTO_ENCAP = 98,	   /* Encapsulation Header.  */
    IPPROTO_PIM = 103,	   /* Protocol Independent Multicast.  */
    IPPROTO_COMP = 108,	   /* Compression Header Protocol.  */
    IPPROTO_L2TP = 115,	   /* Layer 2 Tunnelling Protocol.  */
    IPPROTO_SCTP = 132,	   /* Stream Control Transmission Protocol.  */
    IPPROTO_UDPLITE = 136, /* UDP-Lite protocol.  */
    IPPROTO_MPLS = 137,    /* MPLS in IP.  */
    IPPROTO_ETHERNET = 143, /* Ethernet-within-IPv6 Encapsulation.  */
    IPPROTO_RAW = 255,	   /* Raw IP packets.  */
    IPPROTO_MPTCP = 262,   /* Multipath TCP connection.  */
    IPPROTO_MAX
  };

typedef uint16_t in_port_t;

enum
  {
    IPPORT_ECHO = 7,		/* Echo service.  */
    IPPORT_DISCARD = 9,		/* Discard transmissions service.  */
    IPPORT_SYSTAT = 11,		/* System status service.  */
    IPPORT_DAYTIME = 13,	/* Time of day service.  */
    IPPORT_NETSTAT = 15,	/* Network status service.  */
    IPPORT_FTP = 21,		/* File Transfer Protocol.  */
    IPPORT_TELNET = 23,		/* Telnet protocol.  */
    IPPORT_SMTP = 25,		/* Simple Mail Transfer Protocol.  */
    IPPORT_TIMESERVER = 37,	/* Timeserver service.  */
    IPPORT_NAMESERVER = 42,	/* Domain Name Service.  */
    IPPORT_WHOIS = 43,		/* Internet Whois service.  */
    IPPORT_MTP = 57,

    IPPORT_TFTP = 69,		/* Trivial File Transfer Protocol.  */
    IPPORT_RJE = 77,
    IPPORT_FINGER = 79,		/* Finger service.  */
    IPPORT_TTYLINK = 87,
    IPPORT_SUPDUP = 95,		/* SUPDUP protocol.  */


    IPPORT_EXECSERVER = 512,	/* execd service.  */
    IPPORT_LOGINSERVER = 513,	/* rlogind service.  */
    IPPORT_CMDSERVER = 514,
    IPPORT_EFSSERVER = 520,

    IPPORT_BIFFUDP = 512,
    IPPORT_WHOSERVER = 513,
    IPPORT_ROUTESERVER = 520,
    IPPORT_RESERVED = 1024,
    IPPORT_USERRESERVED = 5000
  };

#define	IN_CLASSA(a)		((((in_addr_t)(a)) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		(0xffffffff & ~IN_CLASSA_NET)
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(a)		((((in_addr_t)(a)) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		(0xffffffff & ~IN_CLASSB_NET)
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(a)		((((in_addr_t)(a)) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		(0xffffffff & ~IN_CLASSC_NET)

#define	IN_CLASSD(a)		((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(a)		IN_CLASSD(a)

#define	IN_EXPERIMENTAL(a)	((((in_addr_t)(a)) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(a)		((((in_addr_t)(a)) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		((in_addr_t) 0x00000000)
#define	INADDR_BROADCAST	((in_addr_t) 0xffffffff)
#define	INADDR_NONE		((in_addr_t) 0xffffffff)
#define	INADDR_DUMMY		((in_addr_t) 0xc0000008)

#define INADDR_UNSPEC_GROUP	((in_addr_t) 0xe0000000) /* 224.0.0.0 */
#define INADDR_ALLHOSTS_GROUP	((in_addr_t) 0xe0000001) /* 224.0.0.1 */
#define INADDR_ALLRTRS_GROUP    ((in_addr_t) 0xe0000002) /* 224.0.0.2 */
#define INADDR_ALLSNOOPERS_GROUP ((in_addr_t) 0xe000006a) /* 224.0.0.106 */
#define INADDR_MAX_LOCAL_GROUP  ((in_addr_t) 0xe00000ff) /* 224.0.0.255 */

#define INET_ADDRSTRLEN 16

#define  __bswap_32(x)	((((x)>>24)&0xff) | \
					(((x)<<8)&0xff0000) | \
					(((x)>>8)&0xff00) | \
					(((x)<<24)&0xff000000))  

#define  __bswap_16(x)	((((x)>>8)&0xff) | \
					(((x)<<8)&0xff00))

#define ntohl(x)	__bswap_32 (x)
#define ntohs(x)	__bswap_16 (x)
#define htonl(x)	__bswap_32 (x)
#define htons(x)	__bswap_16 (x)



#endif	/* netinet/in.h */
