#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#define BUFFER_SIZE 1024

bool _ended = false;
int sockfd = -1;

void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    int n;
    
    while (!_ended) {
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        } else if (n <= 0) {
            printf("\nConnection closed by remote host.\n");
            break;
        } 
    }
    _ended = true;
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t tid;
    int n;
    struct hostent *he;
    char buf[BUFFER_SIZE];
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(1);
    }
    
    setbuf(stdout, NULL);
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    // 尝试通过域名解析
    he = gethostbyname(argv[1]);
    if (he == NULL) {
        // 尝试直接解析 IP 地址
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "Invalid address or hostname: %s\n", argv[1]);
            return -1;
        }
    } else {
        // 使用解析得到的 IP 地址
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    // 创建 socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3; // 3秒超时
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }
    
    // 显示连接信息
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr.sin_addr, addr_str, INET_ADDRSTRLEN);
    if (he != NULL) {
        printf("Connecting to %s (%s):%s ... ", argv[1], addr_str, argv[2]);
    } else {
        printf("Connecting to %s:%s ... ", argv[1], argv[2]);
    }
    
    // 连接服务器
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("failed\n");
        close(sockfd);
        return -1;
    }
    printf("ok %d\n", sockfd);
    
    // 创建接收线程
    if (pthread_create(&tid, NULL, receive_thread, NULL) != 0) {
        close(sockfd);
        return -1;
    }
    
    // 主线程用于发送数据
    while (!_ended) {
        n = read(STDIN_FILENO, buf, BUFFER_SIZE - 1);
        if (n > 0) {
            // 特殊字符处理
            /*
            if (c == 0x1D) { // Ctrl+]
                printf("\nExiting...\n");
                _ended = true;
                break;
            } else if (c == 0x03) { // Ctrl+C
                printf("\nExiting...\n");
                _ended = true;
                break;
            } else if (c == 0x04) { // Ctrl+D
                printf("\nEOF received. Exiting...\n");
                _ended = true;
                break;
            }
            */
            // 正常字符发送
            if (write(sockfd, buf, n) < 0) {
                _ended = true;
                break;
            }
        } 
        else {
            printf("\nEOF received. Exiting...\n");
            _ended = true;
            break;
        }
        sleep(0);
    }
    // 关闭连接
    close(sockfd);

    while (!_ended) {
        usleep(10000);
    }

    return 0;
}
