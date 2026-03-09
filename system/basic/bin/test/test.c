#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/vdevice.h>
#include <time.h>

int main(int argc, char *argv[]) {
    struct tm time_info;
    proto_t out;
    PF->init(&out);
    if(dev_cntl("/dev/localtime", 0, NULL, &out) != 0)
        return -1;
    int res = proto_read_int(&out);
    if(res == 0)
        proto_read_to(&out, &time_info, sizeof(time_info));
    PF->clear(&out);
    if(res != 0)
        return -1;

    char time_string[64] = {0};
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &time_info);
    printf("%s\n", time_string);
    return 0;
}
