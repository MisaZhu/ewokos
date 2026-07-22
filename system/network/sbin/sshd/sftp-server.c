#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ewoksys/vfs.h>

#include "sftp-core.h"

static void notify_parent_ready(void) {
    uint8_t marker = 'R';

    (void)write(VFS_BACKUP_FD1, &marker, 1);
    close(VFS_BACKUP_FD1);
}

static int emit_stdout(void *user, const uint8_t *data, size_t len) {
    int fd = *(int *)user;
    size_t total = 0;

    while (total < len) {
        ssize_t w = write(fd, data + total, len - total);
        if (w < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        total += (size_t)w;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    sftp_server_t *srv;
    sftp_server_io_t io;
    uint8_t buf[16384];
    int out_fd = STDOUT_FILENO;

    (void)argc;
    (void)argv;

    io.emit = emit_stdout;
    io.user = &out_fd;
    srv = sftp_server_create(&io);
    if (!srv)
        return 1;

    notify_parent_ready();
    while (1) {
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (n == 0)
            break;
        if (sftp_server_feed(srv, buf, (size_t)n) < 0)
            break;
    }

    sftp_server_destroy(srv);
    return 0;
}
