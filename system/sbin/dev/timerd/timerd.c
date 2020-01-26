#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <dev/device.h>
#include <sys/interrupt.h>

typedef struct {
	int fd;
	int pid;
	int msec;
	int counter;
} timer_t;

#define TIMER_MAX 32
static timer_t _timers[TIMER_MAX];

static timer_t* get_timer(int fd, int pid) {
	for(int i=0; i<TIMER_MAX; i++) {
		if(_timers[i].fd == fd && _timers[i].pid == pid)
			return &_timers[i];
	}
	return NULL;
}

static timer_t* reg_timer(int fd, int pid) {
	for(int i=0; i<TIMER_MAX; i++) {
		if(_timers[i].fd == 0 && _timers[i].pid == 0) {
			_timers[i].fd = fd;
			_timers[i].pid = pid;
			_timers[i].msec = 0;
			return &_timers[i];
		}
	}
	return NULL;
}

static void unreg_timer(int fd, int pid) {
	timer_t* t = get_timer(fd, pid);
	if(t == NULL)
		return;
	memset(t, 0, sizeof(timer_t));
}

static int timer_open(int fd, 
		int from_pid,
		fsinfo_t* info, 
		int oflag,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)oflag;
	(void)p;

	if(reg_timer(fd, from_pid) == NULL)
		return -1;
	return 0;
}

static int timer_closed(int fd, 
		int from_pid,
		fsinfo_t* info, 
		void* p) {

	(void)info;
	(void)p;

	unreg_timer(fd, from_pid);
	return 0;
}

static int timer_write(int fd,
		int from_pid, 
		fsinfo_t* info, 
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)info;
	(void)offset;
	(void)p;

	if(size != 4) 
		return 0;
	timer_t* t = get_timer(fd, from_pid);
	if(t == NULL)
		return 0;

	uint32_t msec = *(uint32_t*)buf;
	t->counter = msec;	
	t->msec = msec;	
	return 4;
}

void int_func(int int_id, void* p) {
	(void)p;
	if(int_id == US_INT_TIMER_MTIC) {
		for(int i=0; i<TIMER_MAX; i++) {
			timer_t* t = &_timers[i];
			if(t->fd > 0 && t->pid > 0 && t->msec > 0) {
				t->counter--;
				if(t->counter <= 0) {
					t->counter = t->msec;
					proc_interrupt(t->pid, US_INT_TIMER_MTIC);
				}
			}
		}
	}
}

int main(int argc, char** argv) {
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/timer0";
	memset(_timers, 0, sizeof(timer_t)*TIMER_MAX);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "timer");
	dev.open = timer_open;
	dev.closed = timer_closed;
	dev.write = timer_write;

	proc_interrupt_setup(int_func, NULL);
	proc_interrupt_register(US_INT_TIMER_MTIC);
		
	device_run(&dev, mnt_name, FS_TYPE_DEV, NULL, 1);
	return 0;
}
