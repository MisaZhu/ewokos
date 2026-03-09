#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    struct hostent *host = gethostbyname("www.baidu.com");
    if (host == NULL) {
        return -1;
    }

    printf("Host name: %s\n", host->h_name);
    printf("Aliases: ");
    char **alias = host->h_aliases;
    while (*alias != NULL) {
        printf("%s ", *alias);
        alias++;
    }
    printf("\n");
    
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
