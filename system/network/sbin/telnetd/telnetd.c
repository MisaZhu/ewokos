#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

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
    if(input_pipe != NULL) {
        close_fd_if_valid(&clnt_sock);
    }

    setenv("CONSOLE_ID", "telnet");
    if(proc_exec("/bin/login") < 0)
        exit(-1);
    exit(0);
}

static void telnet_session_relay(int clnt_sock, int input_write_fd, int output_read_fd) {
    uint8_t buf[BUF_SIZE];
    int socket_open = 1;
    int shell_input_open = (input_write_fd >= 0) ? 1 : 0;

    while(1) {
        struct pollfd pfds[2];
        int nfds = 0;

        if(socket_open) {
            pfds[nfds].fd = clnt_sock;
            pfds[nfds].events = POLLIN | POLLHUP | POLLERR;
            pfds[nfds].revents = 0;
            nfds++;
        }
        if(output_read_fd >= 0) {
            pfds[nfds].fd = output_read_fd;
            pfds[nfds].events = POLLIN | POLLHUP | POLLERR;
            pfds[nfds].revents = 0;
            nfds++;
        }
        if(nfds == 0) {
            break;
        }

        int pret = poll(pfds, nfds, -1);
        if(pret < 0) {
            if(errno == EINTR) {
                continue;
            }
            break;
        }

        int idx = 0;
        if(socket_open) {
            short revents = pfds[idx++].revents;
            if((revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                ssize_t ret = read(clnt_sock, buf, sizeof(buf));
                if(ret > 0) {
                    if(shell_input_open && write_all(input_write_fd, buf, (size_t)ret) != 0) {
                        close_fd_if_valid(&input_write_fd);
                        shell_input_open = 0;
                    }
                } else {
                    /* Peer closed or socket failed: deliver EOF to the shell. */
                    socket_open = 0;
                    if(shell_input_open) {
                        close_fd_if_valid(&input_write_fd);
                        shell_input_open = 0;
                    }
                }
            }
        }

        if(output_read_fd >= 0) {
            short revents = pfds[idx].revents;
            if((revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                ssize_t ret = read(output_read_fd, buf, sizeof(buf));
                if(ret > 0) {
                    if(!socket_open || write_all(clnt_sock, buf, (size_t)ret) != 0) {
                        socket_open = 0;
                        if(shell_input_open) {
                            close_fd_if_valid(&input_write_fd);
                            shell_input_open = 0;
                        }
                    }
                } else {
                    /* Shell exited: tear the network side down now. */
                    break;
                }
            }
        }
    }

    close_fd_if_valid(&input_write_fd);
    close_fd_if_valid(&output_read_fd);
    close_fd_if_valid(&clnt_sock);
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
        /* Parent becomes the login shell; the relay child owns the socket. */
        become_login_process(serv_sock, clnt_sock, input_pipe, output_pipe);
        return;
    }

    /* Relay child: socket <-> shell pipes. */
    close_fd_if_valid(&serv_sock);
    close_fd_if_valid(&input_pipe[0]);
    close_fd_if_valid(&output_pipe[1]);
    telnet_session_relay(clnt_sock, input_pipe[1], output_pipe[0]);
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
