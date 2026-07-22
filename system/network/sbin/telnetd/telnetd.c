#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
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
#define TELNET_SB   250
#define TELNET_SE   240

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

static int write_all(int fd, const uint8_t *buf, size_t len);
static int write_all_nb(int fd, const uint8_t *buf, size_t len);

static void close_fd_if_valid(int *fd) {
    if(fd != NULL && *fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

static int write_all(int fd, const uint8_t *buf, size_t len) {
    size_t sent = 0;

    while(sent < len) {
        ssize_t ret = write(fd, buf + sent, len - sent);
        if(ret > 0) {
            sent += (size_t)ret;
            continue;
        }
        if(ret < 0 && errno == EINTR)
            continue;
        return -1;
    }
    return 0;
}

typedef struct {
    int clnt_sock;
    int output_prev_cr;
} telnet_relay_t;

static size_t telnet_encode_output(telnet_relay_t *relay, uint8_t *dst,
        const uint8_t *src, size_t len) {
    size_t out = 0;

    for(size_t i = 0; i < len; i++) {
        uint8_t c = src[i];

        if(relay->output_prev_cr) {
            relay->output_prev_cr = 0;
            if(c == '\n') {
                dst[out++] = '\r';
                dst[out++] = '\n';
                continue;
            }
            dst[out++] = '\r';
            dst[out++] = '\0';
        }

        if(c == '\r') {
            relay->output_prev_cr = 1;
            continue;
        }
        if(c == '\n') {
            dst[out++] = '\r';
            dst[out++] = '\n';
            continue;
        }
        if(c == TELNET_IAC) {
            dst[out++] = TELNET_IAC;
            dst[out++] = TELNET_IAC;
            continue;
        }
        dst[out++] = c;
    }
    return out;
}

static int telnet_flush_output_tail(telnet_relay_t *relay) {
    static const uint8_t cr_nul[2] = {'\r', '\0'};

    if(!relay->output_prev_cr)
        return 0;

    relay->output_prev_cr = 0;
    return write_all_nb(relay->clnt_sock, cr_nul, sizeof(cr_nul));
}

static int write_all_nb(int fd, const uint8_t *buf, size_t len) {
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
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            poll(&pfd, 1, 100);
            continue;
        }
        return -1;
    }
    return 0;
}

/*
 * Single-task relay: pump shell stdout -> socket.
 *
 * The input direction needs no relay at all: the client socket is used
 * directly as the shell's stdin and both /bin/login and /bin/shell strip
 * telnet protocol (IAC negotiation + CR translation) themselves when
 * CONSOLE_ID=telnet.
 *
 * The relay must never outlive the login session. Its normal exit path is
 * pipe EOF (vfsd reaps the shell's write ends), but that chain crosses
 * core's polled kevent queue and vfsd's deferred zombie cleanup; a lost
 * step there would park this task on the pipe forever and leak the netd
 * connection task. So instead of a bare blocking read, poll with a timeout
 * (one cheap wakeup per second, no idle busy loop) and watchdog the shell
 * pid: once the shell is gone, drain whatever output remains and exit.
 */
static void telnet_output_relay(int clnt_sock, int output_read_fd, int shell_pid) {
    telnet_relay_t relay;
    uint8_t buf[BUF_SIZE];
    uint8_t outbuf[BUF_SIZE * 2];
    memset(&relay, 0, sizeof(relay));
    relay.clnt_sock = clnt_sock;

    /* The relay task must only ever park on the pipe node, never on the
     * socket node (kernel wakeups are per-node), so keep the socket
     * non-blocking; write_all_nb() waits on POLLOUT when the window is full. */
    int flags = fcntl(clnt_sock, F_GETFL, 0);
    if(flags >= 0)
        fcntl(clnt_sock, F_SETFL, flags | O_NONBLOCK);

    bool shell_dead = false;
    while(1) {
        if(!shell_dead && shell_pid > 0 && proc_get_uuid(shell_pid) == 0)
            shell_dead = true;

        struct pollfd pfd;
        pfd.fd = output_read_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, shell_dead ? 200 : 1000);
        if(pr > 0 && (pfd.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) != 0) {
            errno = 0;
            ssize_t ret = read(output_read_fd, buf, sizeof(buf));
            if(ret > 0) {
                size_t out_len = telnet_encode_output(&relay, outbuf, buf, (size_t)ret);
                if(out_len > 0 && write_all_nb(clnt_sock, outbuf, out_len) != 0)
                    break;
                continue;
            }
            if(ret < 0 && errno == EINTR)
                continue;
            /* Shell exited (pipe EOF) or pipe error: flush a trailing CR and finish. */
            (void)telnet_flush_output_tail(&relay);
            break;
        }
        if(pr < 0)
            break;
        /* Poll timeout: with the shell gone and nothing left to forward,
         * leave so the socket ref returns and netd can release the task. */
        if(shell_dead) {
            (void)telnet_flush_output_tail(&relay);
            break;
        }
    }

    close_fd_if_valid(&output_read_fd);
    close_fd_if_valid(&clnt_sock);
}

static void telnet_send_initial_negotiation(int fd) {
    static const uint8_t init_cmds[] = {
        TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO,
        TELNET_IAC, TELNET_WILL, TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DO,   TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DONT, TELNET_OPT_LINEMODE,
    };

    (void)write_all(fd, init_cmds, sizeof(init_cmds));
}

static void telnet_reply_option_raw(int fd, uint8_t verb, uint8_t opt) {
    uint8_t reply[3] = { TELNET_IAC, 0, opt };
    int len = 3;
    int supported = (opt == TELNET_OPT_ECHO || opt == TELNET_OPT_SUPPRESS_GA);

    switch(verb) {
    case TELNET_DO:
        if(!supported)
            reply[1] = TELNET_WONT;
        else
            len = 0;
        break;
    case TELNET_WILL:
        if(!supported)
            reply[1] = TELNET_DONT;
        else
            len = 0;
        break;
    case TELNET_DONT:
    case TELNET_WONT:
    default:
        len = 0;
        break;
    }

    if(len > 0)
        (void)write_all(fd, reply, (size_t)len);
}

static void telnet_drain_initial_negotiation(int fd) {
    enum {
        NEG_STATE_DATA = 0,
        NEG_STATE_IAC,
        NEG_STATE_CMD,
        NEG_STATE_SB,
        NEG_STATE_SB_DATA,
        NEG_STATE_SB_IAC,
    };
    uint8_t buf[128];
    int state = NEG_STATE_DATA;
    uint8_t verb = 0;
    int idle_rounds = 0;

    while(idle_rounds < 4) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pret = poll(&pfd, 1, 50);
        if(pret <= 0 || !(pfd.revents & POLLIN)) {
            idle_rounds++;
            continue;
        }
        idle_rounds = 0;

        ssize_t ret = read(fd, buf, sizeof(buf));
        if(ret <= 0)
            break;

        for(ssize_t i = 0; i < ret; i++) {
            uint8_t c = buf[i];

            switch(state) {
            case NEG_STATE_DATA:
                if(c == TELNET_IAC)
                    state = NEG_STATE_IAC;
                break;
            case NEG_STATE_IAC:
                if(c == TELNET_IAC) {
                    state = NEG_STATE_DATA;
                }
                else if(c == TELNET_SB) {
                    state = NEG_STATE_SB;
                }
                else if(c == TELNET_WILL || c == TELNET_WONT ||
                        c == TELNET_DO || c == TELNET_DONT) {
                    verb = c;
                    state = NEG_STATE_CMD;
                }
                else {
                    state = NEG_STATE_DATA;
                }
                break;
            case NEG_STATE_CMD:
                telnet_reply_option_raw(fd, verb, c);
                state = NEG_STATE_DATA;
                break;
            case NEG_STATE_SB:
                state = NEG_STATE_SB_DATA;
                break;
            case NEG_STATE_SB_DATA:
                if(c == TELNET_IAC)
                    state = NEG_STATE_SB_IAC;
                break;
            case NEG_STATE_SB_IAC:
                if(c == TELNET_SE)
                    state = NEG_STATE_DATA;
                else if(c != TELNET_IAC)
                    state = NEG_STATE_SB_DATA;
                break;
            default:
                state = NEG_STATE_DATA;
                break;
            }
        }
    }
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
    int db0 = dup2(input_fd, VFS_BACKUP_FD0);
    int db1 = dup2(clnt_sock, VFS_BACKUP_FD1);
    if(d0 < 0 || d1 < 0 || d2 < 0 || db0 < 0 || db1 < 0) {
        exit(-1);
    }
    if(input_pipe != NULL) {
        close_fd_if_valid(&input_pipe[0]);
    }
    if(output_pipe != NULL) {
        close_fd_if_valid(&output_pipe[1]);
    }
    /* Socket-as-stdin (input_pipe == NULL): keep clnt_sock open so netd's
     * per-fd task cache stays valid for post-exec reads on fd 0. */
    if(input_pipe != NULL) {
        close_fd_if_valid(&clnt_sock);
    }

    setenv("CONSOLE_ID", "telnet");
    if(proc_exec("/bin/login") < 0)
        exit(-1);
    exit(0);
}

static void run_telnet_worker(int serv_sock, int clnt_sock) {
    int output_pipe[2] = {-1, -1};
    pid_t pid;

    telnet_send_initial_negotiation(clnt_sock);
    telnet_drain_initial_negotiation(clnt_sock);
    if(pipe(output_pipe) != 0) {
        close_fd_if_valid(&output_pipe[0]);
        close_fd_if_valid(&output_pipe[1]);
        become_login_process(serv_sock, clnt_sock, NULL, NULL);
        return;
    }

    /* The parent of this fork becomes the login shell; remember its pid so
     * the relay child can watchdog the session's lifetime. */
    int shell_pid = getpid();
    pid = fork();
    if(pid < 0) {
        close_fd_if_valid(&output_pipe[0]);
        close_fd_if_valid(&output_pipe[1]);
        become_login_process(serv_sock, clnt_sock, NULL, NULL);
        return;
    }

    if(pid > 0) {
        /* Parent becomes the login shell: stdin is the client socket
         * itself (login/shell strip telnet protocol via CONSOLE_ID=telnet),
         * stdout/stderr go through the output pipe to the relay task. */
        become_login_process(serv_sock, clnt_sock, NULL, output_pipe);
        return;
    }

    /* Relay child: the one and only telnetd task, shell stdout -> socket.
     * Its lifetime is bound to the parent it just forked from — that parent
     * exec()s into /bin/login and then the shell (same pid throughout). */
    close_fd_if_valid(&serv_sock);
    close_fd_if_valid(&output_pipe[1]);
    telnet_output_relay(clnt_sock, output_pipe[0], shell_pid);
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
