#ifndef __TYPES_H__
#define __TYPES_H__

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
//typedef unsigned int		size_t;
typedef unsigned long long	u64;
typedef signed char		s8;
typedef short			s16;
typedef int			s32;
//typedef int			ssize_t;
typedef long long		s64;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

/* sparse defines __CHECKER__; see Documentation/dev-tools/sparse.rst */
#ifdef __CHECKER__
#define __bitwise	__attribute__((bitwise))
#else
#define __bitwise
#endif

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

typedef __signed__ long __s64;
typedef unsigned long __u64;

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))
#define min_t(type, a, b) min(((type) a), ((type) b))
#define max_t(type, a, b) max(((type) a), ((type) b))



#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */

#define	EDEADLK		11	/* Resource deadlock would occur */

#define	EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	EINPROGRESS	36	/* Operation now in progress */
#define	EALREADY	37	/* Operation already in progress */
#define	ENOTSOCK	38	/* Socket operation on non-socket */
#define	EDESTADDRREQ	39	/* Destination address required */
#define	EMSGSIZE	40	/* Message too long */
#define	EPROTOTYPE	41	/* Protocol wrong type for socket */
#define	ENOPROTOOPT	42	/* Protocol not available */
#define	EPROTONOSUPPORT	43	/* Protocol not supported */
#define	ESOCKTNOSUPPORT	44	/* Socket type not supported */
#define	EOPNOTSUPP	45	/* Operation not supported on transport endpoint */
#define	EPFNOSUPPORT	46	/* Protocol family not supported */
#define	EAFNOSUPPORT	47	/* Address family not supported by protocol */
#define	EADDRINUSE	48	/* Address already in use */
#define	EADDRNOTAVAIL	49	/* Cannot assign requested address */
#define	ENETDOWN	50	/* Network is down */
#define	ENETUNREACH	51	/* Network is unreachable */
#define	ENETRESET	52	/* Network dropped connection because of reset */
#define	ECONNABORTED	53	/* Software caused connection abort */
#define	ECONNRESET	54	/* Connection reset by peer */
#define	ENOBUFS		55	/* No buffer space available */
#define	EISCONN		56	/* Transport endpoint is already connected */
#define	ENOTCONN	57	/* Transport endpoint is not connected */
#define	ESHUTDOWN	58	/* Cannot send after transport endpoint shutdown */
#define	ETOOMANYREFS	59	/* Too many references: cannot splice */
#define	ETIMEDOUT	60	/* Connection timed out */
#define	ECONNREFUSED	61	/* Connection refused */
#define	ELOOP		62	/* Too many symbolic links encountered */
#define	ENAMETOOLONG	63	/* File name too long */
#define	EHOSTDOWN	64	/* Host is down */
#define	EHOSTUNREACH	65	/* No route to host */
#define	ENOTEMPTY	66	/* Directory not empty */

#define	EUSERS		68	/* Too many users */
#define	EDQUOT		69	/* Quota exceeded */
#define	ESTALE		70	/* Stale file handle */
#define	EREMOTE		71	/* Object is remote */

#define	ENOLCK		77	/* No record locks available */
#define	ENOSYS		78	/* Function not implemented */

#define	ENOMSG		80	/* No message of desired type */
#define	EIDRM		81	/* Identifier removed */
#define	ENOSR		82	/* Out of streams resources */
#define	ETIME		83	/* Timer expired */
#define	EBADMSG		84	/* Not a data message */
#define	EPROTO		85	/* Protocol error */
#define	ENODATA		86	/* No data available */
#define	ENOSTR		87	/* Device not a stream */

#define	ENOPKG		92	/* Package not installed */

#define	EILSEQ		116	/* Illegal byte sequence */

/* The following are just random noise.. */
#define	ECHRNG		88	/* Channel number out of range */
#define	EL2NSYNC	89	/* Level 2 not synchronized */
#define	EL3HLT		90	/* Level 3 halted */
#define	EL3RST		91	/* Level 3 reset */

#define	ELNRNG		93	/* Link number out of range */
#define	EUNATCH		94	/* Protocol driver not attached */
#define	ENOCSI		95	/* No CSI structure available */
#define	EL2HLT		96	/* Level 2 halted */
#define	EBADE		97	/* Invalid exchange */
#define	EBADR		98	/* Invalid request descriptor */
#define	EXFULL		99	/* Exchange full */
#define	ENOANO		100	/* No anode */
#define	EBADRQC		101	/* Invalid request code */
#define	EBADSLT		102	/* Invalid slot */

#define	EDEADLOCK	EDEADLK

#define	EBFONT		104	/* Bad font file format */
#define	ENONET		105	/* Machine is not on the network */
#define	ENOLINK		106	/* Link has been severed */
#define	EADV		107	/* Advertise error */
#define	ESRMNT		108	/* Srmount error */
#define	ECOMM		109	/* Communication error on send */
#define	EMULTIHOP	110	/* Multihop attempted */
#define	EDOTDOT		111	/* RFS specific error */
#define	EOVERFLOW	112	/* Value too large for defined data type */
#define	ENOTUNIQ	113	/* Name not unique on network */
#define	EBADFD		114	/* File descriptor in bad state */
#define	EREMCHG		115	/* Remote address changed */

#define	EUCLEAN		117	/* Structure needs cleaning */
#define	ENOTNAM		118	/* Not a XENIX named type file */
#define	ENAVAIL		119	/* No XENIX semaphores available */
#define	EISNAM		120	/* Is a named type file */
#define	EREMOTEIO	121	/* Remote I/O error */

#define	ELIBACC		122	/* Can not access a needed shared library */
#define	ELIBBAD		123	/* Accessing a corrupted shared library */
#define	ELIBSCN		124	/* .lib section in a.out corrupted */
#define	ELIBMAX		125	/* Attempting to link in too many shared libraries */
#define	ELIBEXEC	126	/* Cannot exec a shared library directly */
#define	ERESTART	127	/* Interrupted system call should be restarted */
#define	ESTRPIPE	128	/* Streams pipe error */

#define ENOMEDIUM	129	/* No medium found */
#define EMEDIUMTYPE	130	/* Wrong medium type */
#define	ECANCELED	131	/* Operation Cancelled */
#define	ENOKEY		132	/* Required key not available */
#define	EKEYEXPIRED	133	/* Key has expired */
#define	EKEYREVOKED	134	/* Key has been revoked */
#define	EKEYREJECTED	135	/* Key was rejected by service */

/* for robust mutexes */
#define	EOWNERDEAD	136	/* Owner died */
#define	ENOTRECOVERABLE	137	/* State not recoverable */

#define	ERFKILL		138	/* Operation not possible due to RF-kill */

#define EHWPOISON	139	/* Memory page has hardware error */

#define WARN_ON(x)  

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))


#define container_of(ptr, type, member) ({				\
	void *__mptr = (void *)(ptr);					\
	((type *)(__mptr - offsetof(type, member))); })

#define BIT(x) (0x1<<(x))


#define cpu_to_le32(x)  (x)
#define le32_to_cpu(x)  (x)

#define cpu_to_le16(x)  (x)
#define le16_to_cpu(x)  (x)


#define cpu_to_be32(x)  __builtin_bswap32(x)
#define be32_to_cpu(x)  __builtin_bswap32(x)

#define cpu_to_be16(x)  __builtin_bswap16(x)
#define be16_to_cpu(x)  __builtin_bswap16(x)

#define roundup(x, y)   ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) ((x) - ((x) % (y)))

#define __struct_group(TAG, NAME, ATTRS, MEMBERS...) \
	union { \
		struct { MEMBERS } ATTRS; \
		struct TAG { MEMBERS } ATTRS NAME; \
	} ATTRS

#define struct_group_tagged(TAG, NAME, MEMBERS...) \
	__struct_group(TAG, NAME, /* no attrs */, MEMBERS)


#define get_unaligned_le16(x)	((*(uint8_t*)(x))) + ((*(((uint8_t*)(x) + 1))) << 8)


#ifndef setbit
#ifndef NBBY            /* the BSD family defines NBBY */
#define NBBY    8       /* 8 bits per byte */
#endif              /* #ifndef NBBY */
#define setbit(a, i)    (((u8 *)a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define clrbit(a, i)    (((u8 *)a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define isset(a, i) (((const u8 *)a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define isclr(a, i) ((((const u8 *)a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)
#endif              /* setbit */

#define readl(addr) (*((volatile uint32_t *)(addr)))
#define writel(val, addr) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))

#endif
