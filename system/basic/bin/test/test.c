#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ntpc/ntpc.h>

int main(int argc, char *argv[]) {
    time_t current_time = ntpc_get_time(DEFAULT_NTP_SERVER, DEFAULT_NTP_PORT);

    struct tm* time_info = localtime(&current_time);
    if (time_info == NULL) {
        printf("Failed to convert NTP timestamp to local time\n");
        return 1;
    }

    char time_string[64] = {0};
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);
    printf("%s\n", time_string);
    return 0;
}
