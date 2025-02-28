#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/interrupt.h>

#define UART_BASE       (_mmio_base + 0x05000000)
#define UART_LSR        (UART_BASE + 0x14)
#define UART_TX         (UART_BASE + 0x0)

#define UART_LSR_THRE	0x20
#define UART_LSR_DR 	0x01      

#define REG32(x) (*(volatile uint32_t*)(x))

static charbuf_t *_RxBuf;
static charbuf_t *_TxBuf;

static int uart_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)size;
	(void)offset;
	(void)p;

	int i;
    for(i = 0; i < size; i++){
		if(charbuf_pop(_RxBuf, buf + i) != 0)
			break;
    }
    return (i==0)?VFS_ERR_RETRY:i;
}

static int uart_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;

	for(int i = 0 ; i < size; i++){
		while(((REG32(UART_LSR)) & UART_LSR_THRE) == 0)proc_usleep(10);
		REG32(UART_TX) = ((char*)buf)[i];
	}
	return size;
}

static int loop(void* p){
	char c;
	int ret;
	int wake = 0;

	if((REG32(UART_LSR) & UART_LSR_DR)){
		charbuf_push(_RxBuf, REG32(UART_TX), true);
		proc_wakeup(RW_BLOCK_EVT);
	}else{
		proc_usleep(10);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map();
	_TxBuf = charbuf_new(0);
	_RxBuf = charbuf_new(0);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "uart");
	dev.read = uart_read;
	dev.write = uart_write;
	dev.loop_step = loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	charbuf_free(_RxBuf);
	charbuf_free(_TxBuf);

	return 0;
}
