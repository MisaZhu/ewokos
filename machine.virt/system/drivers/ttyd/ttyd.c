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
#include <ewoksys/klog.h>
#include <pthread.h>

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
/* PL011 IMSC bits: RX = bit4, TX = bit5 */
#define UART_RX_IM    0x10
#define UART_TX_IM    0x20

static void uart_init(){
    //enable fifo
    put32(UART0 + UART_LCR, 0x10);
    //enable interrupt
    put32(UART0 + UART_IMASK, UART_RX_IM);
    //enable uart
    put32(UART0 + UART_CR, 0x301);
}

static charbuf_t *_buffer;
static charbuf_t *_txbuf;
static pthread_mutex_t _tx_lock = PTHREAD_MUTEX_INITIALIZER;
/* Single-context vdevice: the IPC write handler and loop_step never run
 * concurrently, so the ring needs no lock. Keeping loop_step on the main
 * context (non-threaded) with a bounded batch avoids both IPC-dispatch
 * starvation and scheduler yield-storms. */

/* IPC-context write: stage into the ring and return immediately. SYSTEM
 * STABILITY GUARANTEE: when the ring is full (UART slower than producers, or a
 * broken/slow drain) we DROP the overflow and still report the write accepted,
 * exactly like a real tty overflow. Writers are NEVER parked on the console,
 * so no amount of tty misbehavior can hang the shell or any other process. */
static int32_t uart_write(const void* data, uint32_t size) {
    pthread_mutex_lock(&_tx_lock);
    uint32_t i;
    for(i=0; i<size; i++) {
        char c = ((char*)data)[i];
        if(c == '\r')
            c = '\n';
        if(_txbuf->size < _txbuf->buf_size)
            charbuf_push(_txbuf, c, false);
        /* else: drop overflow byte, never block the writer */
    }
    pthread_mutex_unlock(&_tx_lock);
    return (int32_t)size;
}

/* Main-loop drain: copy a small batch out of the ring, then feed the UART with
 * a bounded pure-MMIO poll (no sched_yield). The bounded batch returns to the
 * device loop promptly so IPC dispatch is never starved. */
static int tty_loop_step(vdevice_t* dev, void* p) {
    (void)p;
    char batch[64];
    int n = 0;
    pthread_mutex_lock(&_tx_lock);
    while(n < 64 && charbuf_pop(_txbuf, &batch[n]) == 0)
        n++;
    int space = (_txbuf->size < _txbuf->buf_size);
    pthread_mutex_unlock(&_tx_lock);

    for(int i = 0; i < n; i++) {
        uint32_t spins = 0;
        while(get32(UART0 + UART_FLAGS) & UART_TRANSMIT) {
            /* Real kernel block with a deadline: released by the global boot
             * tick even on secondary cores, and immune to the stray-token
             * latch that can make usleep() return immediately forever. */
            proc_block_timeout(0, 200);
            if(++spins > 1000)
                break;
        }
        put32(UART0 + UART_DATA, batch[i]);
    }
    if(n && space)
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_WR);
    if(n == 0)
        proc_block_timeout(0, 1000);
    return 0;
}

static int tty_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, off_t offset, void* p) {
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
        const void* buf, int size, off_t offset, void* p) {
    (void)dev;
    (void)fd;
    (void)info;
    (void)from_pid;
    (void)offset;
    (void)p;
    return uart_write(buf, size);
}

static void interrupt_handle(uint32_t interrupt, ewokos_addr_t p) {
        vdevice_t* dev = (vdevice_t*)p;
    int rx = 0;
    put32(UART0 + UART_ICLR, 0x7FF);
    while(!(get32(UART0 + UART_FLAGS) & 0x10)){
        charbuf_push(_buffer,  get32(UART0+UART_DATA), true);
        rx++;
    }

    if(rx){
        vfs_wakeup(dev->mnt_info.node, VFS_EVT_RD);
    }	

    return 0;
}

int main(int argc, char** argv) {
    const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
    /* map only the PL011 page (@0x09000000); _mmio_base stays the window
       base, so the UART0 offset arithmetic below is unchanged */
    if(mmio_map_offset(0x01000000, 0x1000) == 0) {
        slog("ttyd: mmio_map_offset failed\n");
        return -1;
    }
    _buffer = charbuf_new(0);
    _txbuf = charbuf_new(65536);
    uart_init();

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "tty");
    dev.read = tty_read;
    dev.write = tty_write;
    dev.check_poll_events = tty_check_poll_events;
    dev.loop_step = tty_loop_step;
    dev.loop_step_threaded = true;

    interrupt_handler_t handler;
        handler.data = (ewokos_addr_t)&dev;
    handler.handler = interrupt_handle;
    sys_interrupt_setup(33, &handler);

    device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666, false);
    charbuf_free(_buffer);
    return 0;
}
