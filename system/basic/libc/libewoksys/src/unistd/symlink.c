#include <unistd.h>
#include <errno.h>

/*
 * EwokOS's VFS has no symbolic links: a directory entry always names a real
 * node, and there is no node type that says "interpret my contents as a path".
 * symlink() therefore always fails, with ENOSYS rather than EPERM, because
 * the operation is unimplemented rather than forbidden - the same answer
 * readlink() gives.
 *
 * Qt reaches this through QFileSystemEngine::createLink() and through
 * qstorageinfo_unix.cpp's /proc/mounts probing.  Both check the return value
 * and fall back, so a clean ENOSYS failure is the behaviour they want.
 */
int symlink(const char *target, const char *linkpath)
{
	(void)target; (void)linkpath;
	errno = ENOSYS;
	return -1;
}
