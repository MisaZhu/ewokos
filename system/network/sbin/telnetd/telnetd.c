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
        TELNET_IAC, TELNET_WILL, TELNET_OPT_ECHO,
        TELNET_IAC, TELNET_WILL, TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DO,   TELNET_OPT_SUPPRESS_GA,
        TELNET_IAC, TELNET_DONT, TELNET_OPT_LINEMODE,
    };

    if(telnet_write_all(fd, init_cmds, sizeof(init_cmds)) < 0) {
        klog("telnetd initial negotiation failed: sock=%d errno=%d\n", fd, errno);
    }
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    int port = (argc > 1) ? atoi(argv[1]):SERVER_PORT;
    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // opt_size = sizeof(option);
    // option = 1;
    // setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &option, opt_size);
    slog("Service Port: %d\n", argc);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (-1 == bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
        slog("bind error.\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(serv_sock, 5)) {
        slog("listen error.\n");
        exit(EXIT_FAILURE);
    }

    slog("Listening on port %d...\n", port);


    clnt_addr_size = sizeof(clnt_addr);
    while (true) {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);
        if(clnt_sock > 0){
            slog("Connection request:%08x :%d %d\n", clnt_addr.sin_addr, ntohs(clnt_addr.sin_port), clnt_sock);

            int pid = fork();
            if(pid < 0) {
                slog("fork failed for sock=%d\n", clnt_sock);
                close(clnt_sock);
                continue;
            }
            if(pid == 0) {//child
                klog("telnetd child begin: sock=%d serv=%d\n", clnt_sock, serv_sock);
                close(serv_sock);
                klog("telnetd child before dup2: sock=%d\n", clnt_sock);
                int r0 = dup2(clnt_sock, 0);
                int r1 = dup2(clnt_sock, 1);
                int r2 = dup2(clnt_sock, 2);
                int rb0 = dup2(clnt_sock, VFS_BACKUP_FD0);
                int rb1 = dup2(clnt_sock, VFS_BACKUP_FD1);
                klog("telnetd child after dup2: sock=%d stdin=%d stdout=%d stderr=%d backup0=%d backup1=%d\n",
                        clnt_sock, r0, r1, r2, rb0, rb1);
                telnet_send_initial_negotiation(STDOUT_FILENO);
                close(clnt_sock);
		        setenv("CONSOLE_ID", "telnet");
                klog("telnetd child exec /bin/shell\n");
                if(proc_exec("/bin/shell") < 0) {
                    klog("telnetd child exec /bin/session failed\n");
                    exit(-1);
                }
            }
            else {
                slog("telnetd parent fork ok: child=%d sock=%d\n", pid, clnt_sock);
                close(clnt_sock);
            }
        }
    }

    close(serv_sock);
    return 0;
}
