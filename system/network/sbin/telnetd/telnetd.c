#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <setenv.h>

#define SERVER_PORT 23
#define BUF_SIZE 1024
#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34
#define TELNET_INPUT_CHUNK 64

/*
 * Bounded re-poll interval for telnet_wait_fd_ready(). Using an infinite
 * poll(-1) here can wedge the relay forever if netd's POLLOUT wakeup edge is
 * lost: network_check_poll_events() suppresses live WR while task->state ==
 * NET_TASK_PROCESS, so a task_wakeup_tcp_writers() edge that races that window
 * (or is cleared by vfs_get_poll_events' stale-RW reconciliation) never
 * re-arrives. A finite timeout lets vfs_poll re-check live socket writability
 * (~10ms cadence) and self-heal without busy-spinning.
 */
#define TELNET_WAIT_POLL_MS 1000

static int telnet_wait_fd_ready(int fd, short events);
static int telnet_write_all(int fd, const uint8_t *buf, size_t len);

static int set_fd_nonblock(int fd) {
    int flags;

    if(fd < 0)
        return -1;
    flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        return -1;
    if((flags & O_NONBLOCK) != 0)
        return 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void close_fd_if_valid(int *fd) {
    if(fd != NULL && *fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static int telnet_wait_fd_ready(int fd, short events) {
    struct pollfd pfd;

    if(fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = (short)(events | POLLERR | POLLHUP | POLLNVAL);

    while(1) {
        int rc = poll(&pfd, 1, TELNET_WAIT_POLL_MS);
        if(rc > 0) {
            if((pfd.revents & POLLNVAL) != 0) {
                errno = EBADF;
                return -1;
            }
            if((pfd.revents & (POLLERR | POLLHUP)) != 0) {
                errno = EIO;
                return -1;
            }
            return 0;
        }
        if(rc < 0 && errno != EINTR)
            return -1;
        /* rc == 0: timeout elapsed; re-poll so a lost POLLOUT edge self-heals. */
    }
}

static int telnet_write_all(int fd, const uint8_t *buf, size_t len) {
    size_t sent = 0;

    while(sent < len) {
        ssize_t ret = write(fd, buf + sent, len - sent);
        if(ret > 0) {
            sent += (size_t)ret;
            continue;
        }
        if(ret < 0 && errno == EINTR)
            continue;
        if(ret < 0 && errno == EAGAIN) {
            if(telnet_wait_fd_ready(fd, POLLOUT) != 0)
                return -1;
            continue;
        }
        return -1;
    }
    return 0;
}

static void telnet_send_initial_negotiation(int fd) {
    static const uint8_t init_cmds[] = {
        TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO,
        TELNET_IAC, TELNET_WILL, TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DO,   TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DONT, TELNET_OPT_LINEMODE,
    };

    (void)telnet_write_all(fd, init_cmds, sizeof(init_cmds));
}

static int telnet_open_server_socket(int port) {
    struct sockaddr_in serv_addr;

    while(1) {
        int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(serv_sock < 0) {
            proc_usleep(200000);
            continue;
        }

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == 0 &&
                listen(serv_sock, 5) == 0) {
            return serv_sock;
        }

        close(serv_sock);
        proc_usleep(200000);
    }
}

static void become_login_process(
        int serv_sock,
        int clnt_sock,
        int input_pipe[2],
        int output_pipe[2]) {
    close_fd_if_valid(&serv_sock);
    int input_fd = clnt_sock;
    int output_fd = clnt_sock;

    if(input_pipe != NULL) {
        close_fd_if_valid(&input_pipe[1]);
        input_fd = input_pipe[0];
    }
    if(output_pipe != NULL) {
        close_fd_if_valid(&output_pipe[0]);
        output_fd = output_pipe[1];
    }

    int d0 = dup2(input_fd, 0);
    int d1 = dup2(output_fd, 1);
    int d2 = dup2(output_fd, 2);
    int db = dup2(input_fd, VFS_BACKUP_FD0);
    if(d0 < 0 || d1 < 0 || d2 < 0 || db < 0) {
        exit(-1);
    }
    close(VFS_BACKUP_FD1);
    if(input_pipe != NULL) {
        close_fd_if_valid(&input_pipe[0]);
    }
    if(output_pipe != NULL) {
        close_fd_if_valid(&output_pipe[1]);
    }
    if(input_pipe != NULL || output_pipe != NULL) {
        close_fd_if_valid(&clnt_sock);
    }

    setenv("CONSOLE_ID", "telnet");
    if(proc_exec("/bin/login") < 0)
        exit(-1);
    exit(0);
}

static int telnet_session_relay(int clnt_sock, int input_write_fd, int output_read_fd) {
    while(1) {
        struct pollfd pfds[2];
        int kinds[2];
        int nfds = 0;
        int rc;

        if(clnt_sock >= 0) {
            pfds[nfds].fd = clnt_sock;
            pfds[nfds].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
            pfds[nfds].revents = 0;
            kinds[nfds++] = 0;
        }
        if(output_read_fd >= 0) {
            pfds[nfds].fd = output_read_fd;
            pfds[nfds].events = POLLIN | POLLHUP | POLLERR | POLLNVAL;
            pfds[nfds].revents = 0;
            kinds[nfds++] = 1;
        }
        if(nfds == 0)
            return 0;

        rc = poll(pfds, nfds, -1);
        if(rc < 0) {
            if(errno == EINTR)
                continue;
            return -1;
        }

        /* Drain child output first so interactive echo does not get stuck behind input. */
        for(int i = 0; i < nfds; i++) {
            if(kinds[i] != 1)
                continue;
            if((pfds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) == 0)
                continue;

            uint8_t buf[BUF_SIZE];
            ssize_t ret = read(output_read_fd, buf, sizeof(buf));
            if(ret > 0) {
                if(clnt_sock >= 0 && telnet_write_all(clnt_sock, buf, (size_t)ret) != 0)
                    return -1;
            }
            else {
                close_fd_if_valid(&output_read_fd);
                if(input_write_fd >= 0)
                    close_fd_if_valid(&input_write_fd);
                if(clnt_sock >= 0)
                    close_fd_if_valid(&clnt_sock);
                return 0;
            }
        }

        for(int i = 0; i < nfds; i++) {
            if(kinds[i] != 0)
                continue;
            if((pfds[i].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) == 0)
                continue;

            uint8_t buf[TELNET_INPUT_CHUNK];
            ssize_t ret = read(clnt_sock, buf, sizeof(buf));
            if(ret > 0) {
                if(input_write_fd >= 0 &&
                        telnet_write_all(input_write_fd, buf, (size_t)ret) != 0) {
                    return -1;
                }
            }
            else {
                close_fd_if_valid(&clnt_sock);
                close_fd_if_valid(&input_write_fd);
            }
        }
    }
}

static void run_telnet_worker(int serv_sock, int clnt_sock) {
    int input_pipe[2] = {-1, -1};
    int output_pipe[2] = {-1, -1};
    pid_t pid;

    telnet_send_initial_negotiation(clnt_sock);
    if(pipe(input_pipe) != 0 || pipe(output_pipe) != 0) {
        close_fd_if_valid(&input_pipe[0]);
        close_fd_if_valid(&input_pipe[1]);
        close_fd_if_valid(&output_pipe[0]);
        close_fd_if_valid(&output_pipe[1]);
        become_login_process(serv_sock, clnt_sock, NULL, NULL);
        return;
    }

    pid = fork();
    if(pid < 0) {
        close_fd_if_valid(&input_pipe[0]);
        close_fd_if_valid(&input_pipe[1]);
        close_fd_if_valid(&output_pipe[0]);
        close_fd_if_valid(&output_pipe[1]);
        become_login_process(serv_sock, clnt_sock, NULL, NULL);
        return;
    }

    if(pid > 0) {
        become_login_process(serv_sock, clnt_sock, input_pipe, output_pipe);
        return;
    }

    close_fd_if_valid(&serv_sock);
    close_fd_if_valid(&input_pipe[0]);
    close_fd_if_valid(&output_pipe[1]);
    (void)set_fd_nonblock(input_pipe[1]);
    (void)set_fd_nonblock(output_pipe[0]);
    (void)telnet_session_relay(clnt_sock, input_pipe[1], output_pipe[0]);
    close_fd_if_valid(&input_pipe[1]);
    close_fd_if_valid(&output_pipe[0]);
    close_fd_if_valid(&clnt_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    int serv_sock, clnt_sock;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    int port = (argc > 1) ? atoi(argv[1]) : SERVER_PORT;
    serv_sock = telnet_open_server_socket(port);

    clnt_addr_size = sizeof(clnt_addr);
    while(1) {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);
        if(clnt_sock > 0) {
            int pid = fork();
            if(pid < 0) {
                close(clnt_sock);
                continue;
            }
            if(pid == 0) {
                proc_detach();
                run_telnet_worker(serv_sock, clnt_sock);
            }
            close(clnt_sock);
        }
    }

    close(serv_sock);
    return 0;
}
