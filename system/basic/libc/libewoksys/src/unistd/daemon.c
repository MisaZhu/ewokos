#include <unistd.h>
#include <fcntl.h>

/*
 * No setsid()/fork() support: detach the calling process as far as the
 * system allows (working directory + stdio redirection).
 */
int daemon(int nochdir, int noclose) {
    if (!nochdir) {
        if (chdir("/") != 0)
            return -1;
    }

    if (!noclose) {
        int fd = open("/dev/null", O_RDWR);
        if (fd < 0)
            return -1;
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            close(fd);
    }
    return 0;
}
