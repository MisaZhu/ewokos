#ifndef EWOKOS_LIBC_SYS_UTSNAME_H
#define EWOKOS_LIBC_SYS_UTSNAME_H

#define _UTSNAME_LENGTH 65

struct utsname {
	char sysname[_UTSNAME_LENGTH];
	char nodename[_UTSNAME_LENGTH];
	char release[_UTSNAME_LENGTH];
	char version[_UTSNAME_LENGTH];
	char machine[_UTSNAME_LENGTH];
};

#ifdef __cplusplus
extern "C" {
#endif

int uname(struct utsname *buf);

#ifdef __cplusplus
}
#endif

#endif
