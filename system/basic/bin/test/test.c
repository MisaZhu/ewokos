#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

bool _ended = false;

void *receive_thread(void *arg) {
    int sock = *((int *)arg);
    char buffer[BUFFER_SIZE] = {0};
    int valread;
    
    valread = read(sock, buffer, BUFFER_SIZE);
    if(valread <= 0) {
        printf("Message recv: %d\n", valread);
    }
    else {
        buffer[valread] = '\0';
        printf("Message recv: %d:%s\n", valread, buffer);
    }
    _ended = true;
    return NULL;
}

int main(int argc, char *argv[]) {
    /*if (argc != 3) {
        fprintf(stderr, "Usage: %s <domain> <port>\n", argv[0]);
        return 1;
    }
        */
    setbuf(stdout, NULL);

    const char *domain = "tcpbin.com";//argv[1];
    int port = 4242;//atoi(argv[2]);
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};

    // Resolve domain name to IP address
    struct hostent *host = gethostbyname(domain);
    if (host == NULL) {
        printf("Failed to resolve domain: %s\n", domain);
        return 1;
    }

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return 1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3; // 3秒超时
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sock);
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    //serv_addr.sin_port = (port);

    memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);
    char resolved_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &serv_addr.sin_addr, resolved_ip, INET_ADDRSTRLEN);
    printf("connect (%s:%s) ... ", domain, resolved_ip);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("failed\n");
        return 1;
    }
    printf("ok\n");

    // Create receive thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &sock) != 0) {
        printf("Failed to create receive thread\n");
        close(sock);
        return 1;
    }

    strcpy(message, "hello\n");
    int sz = write(sock, message, strlen(message));
    printf("Message sent: %d:%s", sz, message);

    while(!_ended)
        usleep(100000);
    close(sock);
    return 0;
}