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

/* memory mapping for the serial port */
#define UART0 (_mmio_base+0x01000000)
/* serial port register offsets */
#define UART_DATA       0x00
#define UART_FLAGS      0x18
#define UART_LCR        0x2C
#define UART_CR         0x30
#define UART_IMASK      0x38
#define UART_ICLR     	0x44

#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

static void uart_init(){
	//enable fifo
	put32(UART0 + UART_LCR, 0x10);
	//enable interrupt
	put32(UART0 + UART_IMASK, 0x1<<4);
	//enable uart
	put32(UART0 + UART_CR, 0x301);
}

static inline void uart_basic_trans(char c) {
  /* wait until transmit buffer is full */
  while(get32(UART0 + UART_FLAGS) & UART_TRANSMIT){
	proc_usleep(0);
  }

  /* write the character */
  if(c == '\r')
    c = '\n';
  put32(UART0+UART_DATA, c);
}

 static int32_t uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_basic_trans(c);
  }
  return i;
}

static charbuf_t *_buffer;

static int tty_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)size;
	(void)p;

	int i;
	for(i = 0; i < size; i++){
		int res = charbuf_pop(_buffer, buf + i);
		if(res != 0)
			break;
	}

	return (i==0)?VFS_ERR_RETRY:i;
}

static uint32_t tty_check_poll_events(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)dev;
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)p;

	if(!charbuf_is_empty(_buffer))
		return VFS_EVT_RD;
	return 0;
}

static int tty_write(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)dev;
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;
	return uart_write(buf, size);
}

static bool _wakeup = false;
static void interrupt_handle(uint32_t interrupt, uint32_t p) {
	vdevice_t* dev = (vdevice_t*)p;
	int rx = 0;
	put32(UART0 + UART_ICLR, 0x7FF);
	while(!(get32(UART0 + UART_FLAGS) & 0x10)){
		charbuf_push(_buffer,  get32(UART0+UART_DATA), true);
		rx++;
	}

	if(rx){
		_wakeup = true;
	}	
	return;
}

int tty_loop(vdevice_t* dev, void* p) {
	(void)p;
	if(_wakeup) {
		vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
		_wakeup = false;
	}
	usleep(3000);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();
	_buffer = charbuf_new(0);
	uart_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "tty");
	dev.read = tty_read;
	dev.write = tty_write;
	dev.loop_step = tty_loop;
	dev.check_poll_events = tty_check_poll_events;

	interrupt_handler_t handler;
	handler.data = (uint32_t)&dev;
	handler.handler = interrupt_handle;
	sys_interrupt_setup(33, &handler);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	charbuf_free(_buffer);
	return 0;
}
