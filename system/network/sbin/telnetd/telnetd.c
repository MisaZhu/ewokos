#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/wait.h>
#include <setenv.h>

#define SERVER_PORT 23
#define BUF_SIZE 1024
#define PIPE_READ 0
#define PIPE_WRITE 1

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

typedef struct {
    int prev_cr;
} telnet_input_state_t;

typedef struct {
    int prev_cr;
} telnet_output_state_t;

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
        int rc = poll(&pfd, 1, -1);
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
    }
}

static int telnet_set_fd_nonblock(int fd) {
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

static ssize_t normalize_telnet_input(
        telnet_input_state_t *state, uint8_t *buf, ssize_t len) {
    ssize_t out = 0;

    for(ssize_t i = 0; i < len; ++i) {
        uint8_t c = buf[i];

        if(state->prev_cr) {
            state->prev_cr = 0;
            if(c == '\n' || c == '\0') {
                buf[out++] = '\n';
                continue;
            }
            buf[out++] = '\n';
        }

        if(c == '\r') {
            state->prev_cr = 1;
            continue;
        }
        buf[out++] = c;
    }

    return out;
}

static ssize_t flush_telnet_input_tail(telnet_input_state_t *state, uint8_t *buf) {
    if(!state->prev_cr)
        return 0;
    state->prev_cr = 0;
    buf[0] = '\n';
    return 1;
}

static ssize_t expand_telnet_output(
        telnet_output_state_t *state,
        const uint8_t *input,
        ssize_t len,
        uint8_t *output) {
    ssize_t out = 0;

    for(ssize_t i = 0; i < len; ++i) {
        uint8_t c = input[i];

        if(state->prev_cr) {
            state->prev_cr = 0;
            if(c == '\n') {
                output[out++] = '\r';
                output[out++] = '\n';
                continue;
            }
            output[out++] = '\r';
        }

        if(c == '\r') {
            state->prev_cr = 1;
            continue;
        }
        if(c == '\n') {
            output[out++] = '\r';
            output[out++] = '\n';
            continue;
        }
        output[out++] = c;
    }

    return out;
}

static ssize_t flush_telnet_output_tail(telnet_output_state_t *state, uint8_t *buf) {
    if(!state->prev_cr)
        return 0;
    state->prev_cr = 0;
    buf[0] = '\r';
    return 1;
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

static void run_session_child(
        int serv_sock,
        int clnt_sock,
        int child_stdin[2],
        int child_stdout[2]) {
    close_fd_if_valid(&serv_sock);
    close_fd_if_valid(&clnt_sock);
    close_fd_if_valid(&child_stdin[PIPE_WRITE]);
    close_fd_if_valid(&child_stdout[PIPE_READ]);

    if(dup2(child_stdin[PIPE_READ], 0) < 0 ||
            dup2(child_stdout[PIPE_WRITE], 1) < 0 ||
            dup2(child_stdout[PIPE_WRITE], 2) < 0 ||
            dup2(child_stdin[PIPE_READ], VFS_BACKUP_FD0) < 0) {
        exit(-1);
    }
    close(VFS_BACKUP_FD1);

    close_fd_if_valid(&child_stdin[PIPE_READ]);
    close_fd_if_valid(&child_stdout[PIPE_WRITE]);

    setenv("CONSOLE_ID", "telnet");
    if(proc_exec("/bin/login") < 0)
        exit(-1);
    exit(0);
}

static void run_telnet_worker(int serv_sock, int clnt_sock) {
    int child_stdin[2] = {-1, -1};
    int child_stdout[2] = {-1, -1};
    int child_pid;
    struct pollfd pfds[2];
    telnet_input_state_t input_state = {0};
    telnet_output_state_t output_state = {0};
    uint8_t input_buf[BUF_SIZE];
    uint8_t output_buf[BUF_SIZE];
    uint8_t wire_buf[BUF_SIZE * 2 + 2];
    int child_stdin_open = 1;

    close_fd_if_valid(&serv_sock);

    if(pipe(child_stdin) != 0 || pipe(child_stdout) != 0) {
        close_fd_if_valid(&clnt_sock);
        exit(-1);
    }

    child_pid = fork();
    if(child_pid < 0) {
        close_fd_if_valid(&child_stdin[PIPE_READ]);
        close_fd_if_valid(&child_stdin[PIPE_WRITE]);
        close_fd_if_valid(&child_stdout[PIPE_READ]);
        close_fd_if_valid(&child_stdout[PIPE_WRITE]);
        close_fd_if_valid(&clnt_sock);
        exit(-1);
    }

    if(child_pid == 0)
        run_session_child(serv_sock, clnt_sock, child_stdin, child_stdout);

    close_fd_if_valid(&child_stdin[PIPE_READ]);
    close_fd_if_valid(&child_stdout[PIPE_WRITE]);

    (void)telnet_set_fd_nonblock(clnt_sock);
    (void)telnet_set_fd_nonblock(child_stdin[PIPE_WRITE]);
    (void)telnet_set_fd_nonblock(child_stdout[PIPE_READ]);
    telnet_send_initial_negotiation(clnt_sock);

    while(1) {
        memset(pfds, 0, sizeof(pfds));
        pfds[0].fd = clnt_sock;
        pfds[0].events = child_stdin_open ? (POLLIN | POLLERR | POLLHUP | POLLNVAL) : 0;
        pfds[1].fd = child_stdout[PIPE_READ];
        pfds[1].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

        if(poll(pfds, 2, -1) < 0) {
            if(errno == EINTR)
                continue;
            break;
        }

        if((pfds[1].revents & POLLIN) != 0) {
            ssize_t rd = read(child_stdout[PIPE_READ], output_buf, sizeof(output_buf));
            if(rd > 0) {
                ssize_t wr = expand_telnet_output(&output_state, output_buf, rd, wire_buf);
                if(wr > 0 && telnet_write_all(clnt_sock, wire_buf, (size_t)wr) < 0)
                    break;
            }
            else if(rd == 0) {
                break;
            }
            else if(errno != EINTR && errno != EAGAIN) {
                break;
            }
        }

        if((pfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            ssize_t flush_len = flush_telnet_output_tail(&output_state, wire_buf);
            if(flush_len > 0)
                (void)telnet_write_all(clnt_sock, wire_buf, (size_t)flush_len);
            break;
        }

        if(child_stdin_open && (pfds[0].revents & POLLIN) != 0) {
            ssize_t rd = read(clnt_sock, input_buf, sizeof(input_buf));
            if(rd > 0) {
                rd = normalize_telnet_input(&input_state, input_buf, rd);
                if(rd > 0 && telnet_write_all(child_stdin[PIPE_WRITE], input_buf, (size_t)rd) < 0)
                    break;
            }
            else if(rd == 0) {
                ssize_t flush_len = flush_telnet_input_tail(&input_state, input_buf);
                if(flush_len > 0)
                    (void)telnet_write_all(child_stdin[PIPE_WRITE], input_buf, (size_t)flush_len);
                close_fd_if_valid(&child_stdin[PIPE_WRITE]);
                child_stdin_open = 0;
            }
            else if(errno != EINTR && errno != EAGAIN) {
                break;
            }
        }

        if((pfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            ssize_t flush_len = flush_telnet_input_tail(&input_state, input_buf);
            if(child_stdin_open && flush_len > 0)
                (void)telnet_write_all(child_stdin[PIPE_WRITE], input_buf, (size_t)flush_len);
            close_fd_if_valid(&child_stdin[PIPE_WRITE]);
            child_stdin_open = 0;
        }
    }

    close_fd_if_valid(&child_stdin[PIPE_WRITE]);
    close_fd_if_valid(&child_stdout[PIPE_READ]);
    close_fd_if_valid(&clnt_sock);
    waitpid(child_pid);
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
