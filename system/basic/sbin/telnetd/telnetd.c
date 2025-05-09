#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ewoksys/core.h>
#include <ewoksys/proc.h>
#include <setenv.h>

#define EXIT_FAILURE -1
#define F_OK 0

#define BUF_SIZE 1024
#define SMALL_BUF_SIZE 100

#define SERVER_PORT 23

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size, opt_size;
    int option;

    int port = (argc > 1) ? atoi(argv[1]):SERVER_PORT;
    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // opt_size = sizeof(option);
    // option = 1;
    // setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &option, opt_size);
    printf("Service Port: %d\n", argc);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (-1 == bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) {
        printf("bind error.\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(serv_sock, 5)) {
        printf("listen error.\n");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", port);


    clnt_addr_size = sizeof(clnt_addr);
    while (true) {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);
        if(clnt_sock > 0){
            printf("Connection request:%08x :%d %d\n", clnt_addr.sin_addr, ntohs(clnt_addr.sin_port), clnt_sock);

            int pid = fork();
            if(pid == 0) {//child
                dup2(clnt_sock, 0);
                dup2(clnt_sock, 1);
                dup2(clnt_sock, 2);
                close(clnt_sock);
		        setenv("CONSOLE_ID", "telnet");
                proc_exec("/bin/session");
            }
            else {
                close(clnt_sock);
            }
        }
    }

    close(serv_sock);
    return 0;
}