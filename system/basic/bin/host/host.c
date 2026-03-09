#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <domain>\n", argv[0]);
        return -1;
    }

    struct hostent *host = gethostbyname(argv[1]);
    if (host == NULL) {
        return -1;
    }
    
    // 打印IP地址
    printf("IP addresses: ");
    char **addr_list = host->h_addr_list;
    while (*addr_list != NULL) {
        struct in_addr *addr = (struct in_addr*)*addr_list;
        printf("%s ", inet_ntoa(*addr));
        addr_list++;
    }
    printf("\n");
    
    return 0;
}
