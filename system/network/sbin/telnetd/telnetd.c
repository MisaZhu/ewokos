#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
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

#define TELNET_RELAY_MAX_CLOSE 2

/*
 * Relay one direction of the telnet session using a dedicated thread doing a
 * blocking read() on a SINGLE fd. This deliberately avoids poll()ing the
 * client socket and the shell pipe together: EwokOS's vfs_poll() falls back to
 * a 1ms usleep busy-loop for multi-fd waits (vfsd keeps only one read/one
 * write waiter per pid), which is what burned CPU on an idle telnet session.
 * A single-fd blocking read parks the thread in the kernel via the node-scoped
 * vfs_block path and consumes no CPU while idle.
 */
typedef struct {
    int src;
    int dst;
    int close_fds[TELNET_RELAY_MAX_CLOSE];
    int close_n;
} relay_arg_t;

static int write_all(int fd, const uint8_t *buf, size_t len);

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

static void* relay_thread(void* p) {
    relay_arg_t* a = (relay_arg_t*)p;
    uint8_t buf[BUF_SIZE];

    while(1) {
        ssize_t ret = read(a->src, buf, sizeof(buf));
        if(ret > 0) {
            if(write_all(a->dst, buf, (size_t)ret) != 0)
                break;
            continue;
        }
        if(ret < 0 && errno == EINTR)
            continue;
        break; /* EOF (peer closed) or unrecoverable error */
    }

    for(int i = 0; i < a->close_n; i++)
        close_fd_if_valid(&a->close_fds[i]);
    return NULL;
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
    /*
     * in_arg is static on purpose: the input worker keeps referencing it after
     * this function returns (we do NOT join it), so it must outlive the stack
     * frame. Only one session is relayed per process, so a single static is
     * safe.
     */
    static relay_arg_t in_arg;
    pthread_t in_tid;
    relay_arg_t out_arg;
    int in_started;

    /*
     * Client socket -> shell stdin, on a worker thread. Owns the shell-stdin
     * write end.
     *
     * This thread does a blocking read() on the SOCKET, and a local close()
     * cannot reliably wake a blocked socket read (vfsd only issues a local
     * VFS_EVT_CLOSE wakeup for pipe/anonymous nodes, not sockets). So we must
     * NEVER pthread_join() it: joining could block teardown forever when the
     * shell exits first. Instead the main thread runs the output direction
     * (a PIPE read that always sees EOF once the shell exits), returns, and
     * run_telnet_worker's exit() force-reaps this worker via the kernel
     * (proc_terminate), guaranteeing hang-free teardown in both directions.
     */
    in_arg.src = clnt_sock;
    in_arg.dst = input_write_fd;
    in_arg.close_fds[0] = input_write_fd;
    in_arg.close_n = 1;

    /* Shell stdout -> client socket, on the main thread. Owns the client socket
     * + stdout read end. */
    out_arg.src = output_read_fd;
    out_arg.dst = clnt_sock;
    out_arg.close_fds[0] = clnt_sock;
    out_arg.close_fds[1] = output_read_fd;
    out_arg.close_n = 2;

    in_started = (pthread_create(&in_tid, NULL, relay_thread, &in_arg) == 0);
    if(in_started)
        pthread_detach(in_tid);
    else
        close_fd_if_valid(&input_write_fd); /* shell gets stdin EOF and exits */

    /*
     * Run the output direction here. It returns on shell-stdout pipe EOF (shell
     * exited) or on a socket write error (client gone). Closing clnt_sock sends
     * a FIN so the client notices the session ended and also gives the input
     * worker a best-effort wakeup; either way we return and exit() reaps it.
     */
    (void)relay_thread(&out_arg);
    return 0;
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
    /*
     * telnet_session_relay owns clnt_sock, input_pipe[1] and output_pipe[0]
     * and closes them from its relay threads, so do not close them again here.
     */
    (void)telnet_session_relay(clnt_sock, input_pipe[1], output_pipe[0]);
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
