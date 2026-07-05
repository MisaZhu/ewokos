#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ewoksys/core.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/wait.h>
#include <setenv.h>

#define EXIT_FAILURE -1
#define F_OK 0

#define BUF_SIZE 1024
#define SMALL_BUF_SIZE 100
#define PIPE_READ 0
#define PIPE_WRITE 1

#define SERVER_PORT 23

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

static int telnet_wait_fd_ready(int fd, short events) {
    struct pollfd pfd;

    if(fd < 0) {
        errno = EBADF;
        return -1;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = (short)(events | POLLERR | POLLHUP | POLLNVAL);
    while(true) {
        int rc = poll(&pfd, 1, -1);
        if(rc > 0) {
            if((pfd.revents & POLLNVAL) != 0) {
                errno = EBADF;
                return -1;
            }
            if((pfd.revents & POLLERR) != 0) {
                errno = EIO;
                return -1;
            }
            return 0;
        }
        if(rc == 0)
            continue;
        if(errno != EINTR)
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
        if(ret < 0 && errno == EINTR) {
            continue;
        }
        if(ret < 0 && errno == EAGAIN) {
            if(telnet_wait_fd_ready(fd, POLLOUT) != 0)
                return -1;
            continue;
        }
        return -1;
    }
    return 0;
}

typedef struct {
    int prev_cr;
} telnet_input_state_t;

typedef struct {
    int prev_cr;
} telnet_output_state_t;

typedef struct {
    int clnt_sock;
    int child_stdin_w;
    int child_stdout_r;
    int child_pid;
    volatile int closing;
} telnet_session_t;

static void close_fd_if_valid(int *fd) {
    if(fd != NULL && *fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

/*
 * Wake the main relay loop (blocked in a single-fd poll on the client socket)
 * without disturbing that fd's lifecycle from the main thread. shutdown()
 * issues SOCK_CLOSE to netd, which releases the TCP pcb and raises a
 * VFS read/close wakeup on the socket node, so the blocked poll() returns
 * promptly. The relay loop re-checks the closing flag after waking and exits
 * without issuing another read on the socket.
 *
 * Note: EwokOS shutdown() ignores its "how" argument (netd always performs a
 * full close), and <sys/socket.h> exposes SHUT_* only as self-referential
 * macros, so a plain integer is passed here.
 */
static void telnet_session_close(telnet_session_t* s) {
    if(s == NULL)
        return;
    s->closing = 1;
    if(s->clnt_sock >= 0)
        shutdown(s->clnt_sock, 2 /* SHUT_RDWR, ignored by netd */);
}

static void* telnet_wait_child_thread(void* arg) {
    telnet_session_t* s = (telnet_session_t*)arg;

    if(s == NULL)
        return NULL;
    waitpid(s->child_pid);
    telnet_session_close(s);
    return NULL;
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
    if(!state->prev_cr) {
        return 0;
    }
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
    if(!state->prev_cr) {
        return 0;
    }
    state->prev_cr = 0;
    buf[0] = '\r';
    return 1;
}

static void telnet_send_initial_negotiation(int fd) {
    /*
     * When telnetd execs /bin/shell directly, we must explicitly switch the
     * peer out of line mode. Otherwise some telnet clients only deliver CR/LF
     * cleanly and keep edited command bytes locally, making the session look
     * like the network path is stuck.
     */
    static const uint8_t init_cmds[] = {
        /*
         * Keep echo on the server side so interactive clients do not locally
         * render carriage return as "^M" when the user presses Enter.
         */
        TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO,
        TELNET_IAC, TELNET_WILL, TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DO,   TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DONT, TELNET_OPT_LINEMODE,
    };

    telnet_write_all(fd, init_cmds, sizeof(init_cmds));
}

static int telnet_open_server_socket(int port) {
    struct sockaddr_in serv_addr;

    while (true) {
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

    int r0 = dup2(child_stdin[PIPE_READ], 0);
    int r1 = dup2(child_stdout[PIPE_WRITE], 1);
    int r2 = dup2(child_stdout[PIPE_WRITE], 2);
    int rb0 = dup2(child_stdin[PIPE_READ], VFS_BACKUP_FD0);
    close(VFS_BACKUP_FD1);
    if(r0 < 0 || r1 < 0 || r2 < 0 || rb0 < 0) {
        exit(-1);
    }

    close_fd_if_valid(&child_stdin[PIPE_READ]);
    close_fd_if_valid(&child_stdout[PIPE_WRITE]);
    setenv("CONSOLE_ID", "telnet");
    if(proc_exec("/bin/session") < 0) {
        exit(-1);
    }
    exit(0);
}

static void* telnet_output_thread(void* arg) {
    telnet_session_t* s = (telnet_session_t*)arg;
    telnet_output_state_t output_state = {0};
    uint8_t output_buf[BUF_SIZE];
    uint8_t wire_buf[BUF_SIZE * 2 + 2];

    if(s == NULL)
        return NULL;

    /*
     * Dedicated child-stdout reader (child -> client direction). It only ever
     * waits on a single fd (child_stdout_r) via blocking read() plus a
     * single-fd poll on EAGAIN, so vfsd stays on its efficient kernel-blocking
     * path (proc_block_by) instead of the multi-fd usleep(1000) busy-poll
     * fallback. child stdout EOF means the shell/session exited.
     */
    while(!s->closing) {
        ssize_t rd = read(s->child_stdout_r, output_buf, BUF_SIZE);
        if(rd > 0) {
            ssize_t wr = expand_telnet_output(&output_state, output_buf, rd, wire_buf);
            if(wr > 0 && telnet_write_all(s->clnt_sock, wire_buf, (size_t)wr) < 0)
                break;
            continue;
        }
        if(rd == 0)
            break;
        if(errno == EINTR)
            continue;
        if(errno == EAGAIN) {
            if(telnet_wait_fd_ready(s->child_stdout_r, POLLIN) != 0)
                break;
            continue;
        }
        break;
    }

    {
        ssize_t flush_len = flush_telnet_output_tail(&output_state, wire_buf);
        if(flush_len > 0)
            (void)telnet_write_all(s->clnt_sock, wire_buf, (size_t)flush_len);
    }

    telnet_session_close(s);
    return NULL;
}

static int relay_telnet_session(telnet_session_t* s) {
    struct pollfd pfd;
    telnet_input_state_t input_state = {0};
    uint8_t input_buf[BUF_SIZE];

    /*
     * Main relay loop, client -> child direction only. It polls exactly one
     * fd (the client socket) with an infinite timeout, so vfsd takes the
     * single-waiter kernel-blocking path (proc_block_by) rather than the
     * multi-fd usleep(1000) busy-poll fallback. The child -> client direction
     * runs in telnet_output_thread, and child exit is observed by
     * telnet_wait_child_thread; both wake this loop through
     * telnet_session_close() (a socket shutdown that raises a VFS wakeup).
     */
    while(!s->closing) {
        memset(&pfd, 0, sizeof(pfd));
        pfd.fd = s->clnt_sock;
        pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

        int pret = poll(&pfd, 1, -1);
        if(pret < 0) {
            if(errno == EINTR)
                continue;
            break;
        }
        if(pret == 0)
            continue;

        /*
         * Woken by a helper thread tearing the session down (child exit).
         * Do not touch the socket further; just leave the loop.
         */
        if(s->closing)
            break;

        if((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            ssize_t flush_len = flush_telnet_input_tail(&input_state, input_buf);
            if(flush_len > 0)
                (void)telnet_write_all(s->child_stdin_w, input_buf, (size_t)flush_len);
            break;
        }

        if((pfd.revents & POLLIN) != 0) {
            ssize_t rd = read(s->clnt_sock, input_buf, sizeof(input_buf));
            if(rd < 0) {
                if(errno == EINTR || errno == EAGAIN)
                    continue;
                break;
            } else if(rd == 0) {
                ssize_t flush_len = flush_telnet_input_tail(&input_state, input_buf);
                if(flush_len > 0)
                    (void)telnet_write_all(s->child_stdin_w, input_buf, (size_t)flush_len);
                break;
            } else {
                rd = normalize_telnet_input(&input_state, input_buf, rd);
                if(rd > 0 && telnet_write_all(s->child_stdin_w, input_buf, (size_t)rd) < 0)
                    break;
            }
        }
    }

    s->closing = 1;
    return 0;
}

static void run_telnet_worker(int serv_sock, int clnt_sock) {
    int child_stdin[2] = {-1, -1};
    int child_stdout[2] = {-1, -1};
    telnet_session_t* session = NULL;
    pthread_t output_tid;
    pthread_t wait_tid;

    close_fd_if_valid(&serv_sock);

    if(pipe(child_stdin) != 0 || pipe(child_stdout) != 0) {
        close_fd_if_valid(&clnt_sock);
        exit(-1);
    }

    int pid = fork();
    if(pid < 0) {
        close_fd_if_valid(&child_stdin[PIPE_READ]);
        close_fd_if_valid(&child_stdin[PIPE_WRITE]);
        close_fd_if_valid(&child_stdout[PIPE_READ]);
        close_fd_if_valid(&child_stdout[PIPE_WRITE]);
        close_fd_if_valid(&clnt_sock);
        exit(-1);
    }

    if(pid == 0) {
        run_session_child(serv_sock, clnt_sock, child_stdin, child_stdout);
    }

    close_fd_if_valid(&child_stdin[PIPE_READ]);
    close_fd_if_valid(&child_stdout[PIPE_WRITE]);

    session = (telnet_session_t*)calloc(1, sizeof(*session));
    if(session == NULL) {
        close_fd_if_valid(&child_stdin[PIPE_WRITE]);
        close_fd_if_valid(&child_stdout[PIPE_READ]);
        close_fd_if_valid(&clnt_sock);
        waitpid(pid);
        exit(-1);
    }
    session->clnt_sock = clnt_sock;
    session->child_stdin_w = child_stdin[PIPE_WRITE];
    session->child_stdout_r = child_stdout[PIPE_READ];
    session->child_pid = pid;
    session->closing = 0;

    (void)telnet_set_fd_nonblock(clnt_sock);
    (void)telnet_set_fd_nonblock(child_stdin[PIPE_WRITE]);
    (void)telnet_set_fd_nonblock(child_stdout[PIPE_READ]);
    telnet_send_initial_negotiation(clnt_sock);

    /*
     * Dedicated per-fd helper threads (detached, like sshd): each blocks on a
     * single fd so vfsd never falls into its multi-fd usleep(1000) poll path.
     * The output thread relays child stdout, the wait thread reaps the child.
     */
    if(pthread_create(&output_tid, NULL, telnet_output_thread, session) == 0)
        pthread_detach(output_tid);
    if(pthread_create(&wait_tid, NULL, telnet_wait_child_thread, session) == 0)
        pthread_detach(wait_tid);

    (void)relay_telnet_session(session);

    /*
     * Session over. Mark closing and drop stdin so the child sees EOF and
     * exits (client-disconnect case); that EOFs child stdout and releases the
     * output thread. Helper threads are detached, so exit() below tears down
     * any that are still blocked, mirroring sshd's teardown. The session
     * struct is intentionally left allocated: detached threads may still
     * reference it, and the process exits immediately.
     */
    session->closing = 1;
    session->clnt_sock = -1;
    close_fd_if_valid(&child_stdin[PIPE_WRITE]);
    session->child_stdin_w = -1;
    close_fd_if_valid(&child_stdout[PIPE_READ]);
    session->child_stdout_r = -1;
    close_fd_if_valid(&clnt_sock);
    exit(0);
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;

    int port = (argc > 1) ? atoi(argv[1]):SERVER_PORT;
    serv_sock = telnet_open_server_socket(port);

    clnt_addr_size = sizeof(clnt_addr);
    while (true) {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);
        if(clnt_sock > 0){
            int pid = fork();
            if(pid < 0) {
                close(clnt_sock);
                continue;
            }
            if(pid == 0) {
                run_telnet_worker(serv_sock, clnt_sock);
            }
            else {
                close(clnt_sock);
            }
        }
    }


    close(serv_sock);
    return 0;
}
