#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FAN_LEVEL_MAX 10

int main(int argc, char** argv) {
    unsigned char level;
    int fd;

    setbuf(stdout, NULL);
    fd = open("/dev/fan", O_RDWR);
    if (fd < 0) {
        printf("fan: /dev/fan not found\n");
        return 1;
    }

    if (argc < 2) {
        if (read(fd, &level, 1) != 1) {
            printf("fan: read failed\n");
            close(fd);
            return 1;
        }
        printf("fan level %u duty %u%%\n",
                (unsigned)level, (unsigned)level * 100 / FAN_LEVEL_MAX);
        close(fd);
        return 0;
    }

    int value = atoi(argv[1]);
    if (value < 0)
        value = 0;
    if (value > FAN_LEVEL_MAX)
        value = FAN_LEVEL_MAX;
    level = (unsigned char)value;
    if (write(fd, &level, 1) != 1) {
        printf("fan: write failed\n");
        close(fd);
        return 1;
    }
    if (read(fd, &level, 1) == 1)
        printf("fan level %u duty %u%%\n",
                (unsigned)level, (unsigned)level * 100 / FAN_LEVEL_MAX);
    else
        printf("fan set level %d\n", value);
    close(fd);
    return 0;
}
