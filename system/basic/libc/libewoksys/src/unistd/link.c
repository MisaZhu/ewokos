#include <unistd.h>
#include <errno.h>

/*
 * Hard links.  A hard link needs two things EwokOS's VFS does not have: an
 * inode that can be named by more than one directory entry, and a link count
 * on that inode telling the filesystem when it is finally unreferenced.
 * EwokOS nodes are owned by exactly one parent directory, so link() always
 * fails with ENOSYS - the operation is unimplemented, not forbidden.
 *
 * This is the sibling of symlink() and readlink(), which fail the same way.
 * Qt's QFileSystemEngine::createLink() calls link() for the hard-link case and
 * checks the result, so a clean failure is what it expects.
 */
int link(const char *oldpath, const char *newpath)
{
	(void)oldpath; (void)newpath;
	errno = ENOSYS;
	return -1;
}
