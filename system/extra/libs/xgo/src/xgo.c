#include <xgo/xgo.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

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

static int xgo_send(uint8_t type, uint8_t cmd,uint8_t data) {
   if(tty0FD <= 0)
        return -1;

    uint8_t buf[XGO_DATA_MAX+8];
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
    int ret = write(tty0FD, buf, len);
    return ret;
}

static int xgo_recv(uint8_t cmd, uint8_t len, uint8_t data[XGO_DATA_MAX]) {
    if(len <= 0)
        return -1;

    uint8_t buf[XGO_DATA_MAX+8];
    int rd = read(tty0FD, buf, len + 8);

    if(buf[0] != 0x55 || buf[1] != 0x00 || buf[3] != 0x12) //response
        return -1;

    if(buf[4] != cmd)
        return -1;

    for(uint8_t i=0; i<len; i++) {
        data[i] = buf[5+i];
    }
    return 0;
}

int xgo_cmd(uint8_t type, uint8_t cmd, uint8_t data, uint8_t *res) {
    if(xgo_send(type, cmd, data) < 0) 
        return -1;
    
    if(type != XGO_TYPE_READ || res == NULL) {
        usleep(100000);
        return 0;
    }
    
    int ret = xgo_recv(cmd, data, res);
    usleep(100000);
    return ret;
}


#ifdef __cplusplus
}
#endif