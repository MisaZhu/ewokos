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
#include <sys/fcntl.h>

#define BUFFER_SIZE 1024

bool _ended = false;
int sockfd = -1;

static int handle_cmd(uint8_t *cmd) {
    if(cmd[0] == 0xfb) {
        if(cmd[1] == 0x00) {
            return 0;
        }
    }
    return 1;
}

static void receive_cmd(void) {
    char buffer[BUFFER_SIZE];
    int n;
    while (!_ended) {
        uint8_t c;
        if(read(sockfd, &c, 1) <= 0) {
            _ended = true;
            break;
        }
        if(c != 0xff) {
            write(STDOUT_FILENO, c, 1);
            break;
        }

        uint8_t cmd[2];
        if(read(sockfd, cmd, 2) != 2) {
            _ended = true;
            break;
        }
        int res = handle_cmd(cmd);
        if(res == 0)
            break;
        else if(res < 0) {
            _ended = true;
            break;
        }
    }
}

static void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    int n;
    receive_cmd();
        
    while (!_ended) {
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n > 0) {
            for(int i = 0; i < n; i++) {
                if(buffer[i] != '\r') {
                    write(STDOUT_FILENO, &buffer[i], 1);
                }
            }
        } else if (n <= 0) {
            printf("\nConnection closed by remote host.\n");
            break;
        } 
    }
    _ended = true;
    return NULL;
}

bool sepcial_char_check(uint8_t c) {
    // 特殊字符处理
    if (c == 0x1D) { // Ctrl+]
        printf("\nCtrl+] Exiting connection...\n");
        _ended = true;
        return true;
    } else if (c == 0x04) { // Ctrl+D
        printf("\nCtrl+D Exiting connection...\n");
        _ended = true;
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    pthread_t tid;
    int n;
    struct hostent *he;
    char buf[BUFFER_SIZE];
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [port]\n", argv[0]);
        exit(1);
    }
    
    setbuf(stdout, NULL);
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (argc > 2)
        server_addr.sin_port = htons(atoi(argv[2]));
    else
        server_addr.sin_port = htons(23);

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

    /*struct timeval timeout;
    timeout.tv_sec = 3; // 3秒超时
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sockfd);
        return -1;
    }
    */
    
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
    printf("ok\n");
    
    // 创建接收线程
    if (pthread_create(&tid, NULL, receive_thread, NULL) != 0) {
        close(sockfd);
        return -1;
    }

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    
    // 主线程用于发送数据
    while (!_ended) {
        n = read(STDIN_FILENO, buf, BUFFER_SIZE - 1);
        if (n > 0) {
            buf[n] = '\0';
            for(int i = 0; i < n; i++) {
                if(buf[i] == '\n') {
                    buf[i] = '\r';
                }
                if(sepcial_char_check(buf[i]))
                    break;
            }
            if(_ended)
                break;

            // 正常字符发送
            int sz =  write(sockfd, buf, n);
            if(sz < 0) {
                _ended = true;
                break;
            }
        } 
        /*else if(errno != EAGAIN){
            printf("\nEOF received. Exiting...\n");
            _ended = true;
            break;
        }
        */
        usleep(10000);
    }
    // 关闭连接
    close(sockfd);

    while (!_ended) {
        usleep(10000);
    }

    return 0;
}
