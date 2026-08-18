#include <unistd.h>
#include <pthread.h>
#include <sys/errno.h>

/*
 * pread()/pwrite() are emulated with seek+read/write. The file offset is
 * restored afterwards; the lock keeps concurrent callers from clobbering
 * each other's offsets.
 */
static pthread_mutex_t pio_lock = PTHREAD_MUTEX_INITIALIZER;

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    off_t saved;
    ssize_t ret;

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&pio_lock);
    saved = lseek(fd, 0, SEEK_CUR);
    if (saved < 0 || lseek(fd, offset, SEEK_SET) < 0) {
        pthread_mutex_unlock(&pio_lock);
        errno = EBADF;
        return -1;
    }
    ret = read(fd, buf, count);
    lseek(fd, saved, SEEK_SET);
    pthread_mutex_unlock(&pio_lock);
    return ret;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    off_t saved;
    ssize_t ret;

    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }

    pthread_mutex_lock(&pio_lock);
    saved = lseek(fd, 0, SEEK_CUR);
    if (saved < 0 || lseek(fd, offset, SEEK_SET) < 0) {
        pthread_mutex_unlock(&pio_lock);
        errno = EBADF;
        return -1;
    }
    ret = write(fd, buf, count);
    lseek(fd, saved, SEEK_SET);
    pthread_mutex_unlock(&pio_lock);
    return ret;
}
