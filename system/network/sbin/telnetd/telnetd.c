#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ewoksys/core.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <setenv.h>

#define EXIT_FAILURE -1
#define F_OK 0

#define BUF_SIZE 1024
#define SMALL_BUF_SIZE 100

#define SERVER_PORT 23

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

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
        return -1;
    }
    return 0;
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
            int sync_pipe[2] = {-1, -1};
            if(pipe(sync_pipe) != 0) {
                close(clnt_sock);
                continue;
            }

            int pid = fork();
            if(pid < 0) {
                close(sync_pipe[0]);
                close(sync_pipe[1]);
                close(clnt_sock);
                continue;
            }
            if(pid == 0) {//child
                char ok = 0;
                close(sync_pipe[0]);
                int r0 = dup2(clnt_sock, 0);
                int r1 = dup2(clnt_sock, 1);
                int r2 = dup2(clnt_sock, 2);
                int rb0 = dup2(clnt_sock, VFS_BACKUP_FD0);
                int rb1 = dup2(clnt_sock, VFS_BACKUP_FD1);
                (void)r0;
                (void)r1;
                (void)r2;
                (void)rb0;
                (void)rb1;
                if(r0 < 0 || r1 < 0 || r2 < 0 || rb0 < 0 || rb1 < 0) {
                    write(sync_pipe[1], &ok, 1);
                    close(sync_pipe[1]);
                    close(clnt_sock);
                    close(serv_sock);
                    exit(-1);
                }
                ok = 1;
                write(sync_pipe[1], &ok, 1);
                close(sync_pipe[1]);
                close(serv_sock);
                telnet_send_initial_negotiation(STDOUT_FILENO);
                close(clnt_sock);
		        setenv("CONSOLE_ID", "telnet");
                if(proc_exec("/bin/session") < 0) {
                    exit(-1);
                }
            }
            else {
                char ok = 0;
                close(sync_pipe[1]);
                read(sync_pipe[0], &ok, 1);
                close(sync_pipe[0]);
                close(clnt_sock);
            }
        }
    }


    close(serv_sock);
    return 0;
}
