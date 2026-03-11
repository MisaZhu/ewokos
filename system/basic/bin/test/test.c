#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    /*if (argc != 3) {
        fprintf(stderr, "Usage: %s <domain> <port>\n", argv[0]);
        return 1;
    }
        */

    const char *domain = "tcpbin.com";//argv[1];
    int port = 4242;//atoi(argv[2]);
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char message[BUFFER_SIZE] = {0};

    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    //serv_addr.sin_port = (port);

    // Resolve domain name to IP address
    printf("get host %s, %d, %d\n", domain, port, htons(port));
    struct hostent *host = gethostbyname(domain);
    if (host == NULL) {
        printf("Failed to resolve domain: %s\n", domain);
        return 1;
    }
    printf("get host %s ok\n", domain);

    memcpy(&serv_addr.sin_addr, host->h_addr_list[0], host->h_length);
    char resolved_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &serv_addr.sin_addr, resolved_ip, INET_ADDRSTRLEN);
    printf("connect (%s)\n", resolved_ip);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return 1;
    }
    printf("connected\n");

   // Get input from user
    strcpy(message, "hello\n");
    // Send message to server
    int sz = send(sock, message, strlen(message), 0);
    printf("Message sent: %d:%s", sz, message);

    // Read response from server
    int valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        printf("Echo received: %s", buffer);
    }
    close(sock);
    return 0;
}