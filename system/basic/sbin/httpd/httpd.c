#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ewoksys/proc.h>

#define EXIT_FAILURE -1
#define F_OK 0

#define BUF_SIZE 1024
#define SMALL_BUF_SIZE 100

#define SERVER_NAME "simple httpd\r\n"
#define SERVER_PORT 80

void sgets(char* buf, int size, int sock){
    recv(sock, (void*)buf, size, 0);
}

void send_error_msg(int fd, const int error_code)
{
    char *protocol_and_status;
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[SMALL_BUF_SIZE] = "Content-type:text/html\r\n\r\n";

    char *content;

    char txbuf[1500] = {0};

    if (400 == error_code)
    {
        protocol_and_status = "HTTP/1.0 400 Bad Request\r\n";
        content = "<html><head><title>network</title></head><body>400 Bad Request</body></html>";
    }
    else if (404 == error_code)
    {
        protocol_and_status = "HTTP/1.0 404 Not Found\r\n";
        content = "<html><head><title>network</title></head><body>404 Not Found</body></html>";
    }

    sprintf(content_length, "Content-length:%lu\r\n", strlen(content));

    strcat(txbuf,  protocol_and_status);
    strcat(txbuf, server_name);
    strcat(txbuf, content_length);
    strcat(txbuf, content_type);
    send(fd, txbuf, strlen(txbuf), 0);
    send(fd, content, strlen(content), 0);
}

void send_data_html(int sock)
{
    char txbuf[1500] = {0};
    char protocol_and_status[] = "HTTP/1.0 200 ok\r\nAccept-Ranges: bytes\r\n";
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[] = "Content-Type:text/html\r\n\r\n";
    char *content = "<!DOCTYPE html>  \n\
                     <html>           \n\
                        <body>\n\
                            <p>Hello EwokOS! May the Force be with you.</p>\n\
                        </body>\n\
                        <iframe \n\
  id=\"frame\" \n\
  title=\"picture\" \n\
  width=\"640\"  \n\
  height=\"480\"  \n\
  src=\"https://th.bing.com/th/id/OIP.NSSkqwKCXamGYiNKXnyIjAAAAA?rs=1&pid=ImgDetMain\"> \n\
</iframe> \n\
                    </html>\n";

    FILE *response_file_fp;

    sprintf(content_length, "Content-Length:%ld\r\n", strlen(content));

    strcat(txbuf, protocol_and_status);
    strcat(txbuf, server_name);
    strcat(txbuf, content_length);
    strcat(txbuf, content_type);
    send(sock, txbuf, strlen(txbuf), 0);
    send(sock, content, strlen(content), 0);
}

void *request_handler(void *arg)
{
    int sock = (int)arg;
    char request_msg[SMALL_BUF_SIZE] = {0};

    char http_method[10] = {0};
    char file_name[SMALL_BUF_SIZE] = {0};
    recv(sock, request_msg, SMALL_BUF_SIZE, 0);

    if (NULL == strstr(request_msg, "HTTP/"))
    {
        send_error_msg(sock, 400);
        goto handler_exit; 
    }

    strcpy(http_method, strtok(request_msg, " /"));
    strcpy(file_name, strtok(NULL, " /"));

    if (strcmp(http_method, "GET") != 0)
    {
        send_error_msg(sock, 400);
        goto handler_exit; 
    }

    send_data_html(sock);
handler_exit:
    close(sock);

    return NULL;
}

void test(int i){
    uint8_t tmp[4096];
    memset(tmp, i, sizeof(tmp));
}

void *test_handler(void *arg){
    proc_usleep(1000);
    test((int)arg);
    close((int)arg);
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size, opt_size;
    int option;

    pthread_t t_id;

// while(1){
//     int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if(fd >= 0){
//         //close(fd); 
//         pthread_create(&t_id, NULL, test_handler, (void*)fd);
//     }
//     proc_usleep(10000);
// }
    int port = (argc > 1) ? atoi(argv[1]):80;
    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // opt_size = sizeof(option);
    // option = 1;
    // setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &option, opt_size);
    printf("Service Port: %d\n", argc);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_un.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (-1 == bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
    {
        printf("bind error.\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(serv_sock, 5))
    {
        printf("listen error.\n");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", SERVER_PORT);


    clnt_addr_size = sizeof(clnt_addr);
    while (1)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);
        if(clnt_sock > 0){
            printf("Connection request:%08x :%d %d\n", clnt_addr.sin_addr, ntohs(clnt_addr.sin_port), clnt_sock);
            pthread_create(&t_id, NULL, request_handler, (void*)clnt_sock);
            // request_handler((void*)clnt_sock);
            //close(clnt_sock);
        }
    }

    close(serv_sock);

    return 0;
}