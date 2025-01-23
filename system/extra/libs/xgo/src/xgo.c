#include <xgo/xgo.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

static int tty0FD = -1;

int  xgo_init(void) {
	tty0FD = open("/dev/tty0", O_RDWR);
    if(tty0FD <= 0)
        return -1;
    return 0;
}

void xgo_quit(void) {
    if(tty0FD > 0)
        close(tty0FD);
}

int xgo_send(uint8_t type, uint8_t cmd,uint8_t data) {
   if(tty0FD <= 0)
        return -1;

    uint8_t buf[DATA_MAX+8];
    uint8_t i = 0;
    buf[i] = 0x55; i++;
    buf[i] = 0x0; i++;
    i++; //resv for buf len
    buf[i] = type; i++;
    buf[i] = cmd; i++;
    buf[i] = data; i++;
    i++; //resv for check sum
    buf[i] = 00; i++;
    buf[i] = 0xAA; i++;

    uint8_t len = i;
    buf[2] = len; //buf len

    uint32_t sum = 0;
    for(i=2; i<6; i++)
        sum += buf[i];
    buf[6] = ~(sum); //check sum
    return write(tty0FD, buf, len);
}

int xgo_cmd(uint8_t type, uint8_t cmd,uint8_t data, uint8_t res[DATA_MAX]) {
    if(tty0FD <= 0)
        return -1;

    uint8_t buf[DATA_MAX+8];
    uint8_t i = 0;
    buf[i] = 0x55; i++;
    buf[i] = 0x0; i++;
    i++; //resv for buf len
    buf[i] = type; i++;
    buf[i] = cmd; i++;
    buf[i] = data; i++;
    i++; //resv for check sum
    buf[i] = 00; i++;
    buf[i] = 0xAA; i++;

    uint8_t len = i;
    buf[2] = len; //buf len

    uint32_t sum = 0;
    for(i=2; i<6; i++)
        sum += buf[i];
    buf[6] = ~(sum); //check sum

    return write(tty0FD, buf, len);
}


#ifdef __cplusplus
}
#endif