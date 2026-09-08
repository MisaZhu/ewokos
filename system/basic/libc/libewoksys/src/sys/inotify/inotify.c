#include <sys/inotify.h>
#include <errno.h>

/* EwokOS has no kernel inotify support; every entry point fails with ENOSYS
 * so that applications can detect the absence and disable monitoring. */

int inotify_init(void)
{
	errno = ENOSYS;
	return -1;
}

int inotify_init1(int flags)
{
	(void)flags;
	errno = ENOSYS;
	return -1;
}

int inotify_add_watch(int fd, const char *path, uint32_t mask)
{
	(void)fd; (void)path; (void)mask;
	errno = ENOSYS;
	return -1;
}

int inotify_rm_watch(int fd, int wd)
{
	(void)fd; (void)wd;
	errno = ENOSYS;
	return -1;
}
