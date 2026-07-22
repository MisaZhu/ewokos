#include <sys/utsname.h>
#include <string.h>
#include <errno.h>
#include <ewoksys/sys.h>

static void utsname_copy(char *dst, const char *src) {
	if (src == NULL || src[0] == 0) {
		dst[0] = 0;
		return;
	}
	strncpy(dst, src, _UTSNAME_LENGTH - 1);
	dst[_UTSNAME_LENGTH - 1] = 0;
}

int uname(struct utsname *buf) {
	sys_info_t info;

	if (buf == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(buf, 0, sizeof(*buf));
	if (sys_get_sys_info(&info) != 0) {
		return -1;
	}

	utsname_copy(buf->sysname, "Ewok");
	utsname_copy(buf->nodename, info.machine);
	utsname_copy(buf->release, "0.01");
	utsname_copy(buf->version, info.arch);
	if (info.arch[0] != 0) {
		utsname_copy(buf->machine, info.arch);
	}
	else {
		utsname_copy(buf->machine, info.machine);
	}
	return 0;
}
