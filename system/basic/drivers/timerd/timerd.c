#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/interrupt.h>
#include <sys/kernel_tic.h>

typedef struct interrupt_st {
	int32_t  pid;
	uint32_t entry;
	uint32_t timer_usec;
	uint64_t timer_last;
	struct   interrupt_st* next;
} interrupt_t;
static interrupt_t* _intr_list = NULL;

static int32_t interrupt_setup(int32_t pid, uint32_t timer_usec, uint32_t entry) {
	if(timer_usec == 0 || entry == 0) { //remove pid
		interrupt_t* p = _intr_list;
		interrupt_t* prev = NULL;
		while(p != NULL) {
			interrupt_t* next = p->next;
			if(p->pid == pid) {
				if(p == _intr_list)	
					_intr_list = next;
				if(prev != NULL)
					prev->next = next;
				free(p);
			}
			else {
				prev = p;
			}
			p = next;
		}
		return 0;
	}

	interrupt_t* intr = (interrupt_t*)malloc(sizeof(interrupt_t));
	intr->pid = pid;
	intr->entry = entry;
	intr->timer_usec = timer_usec;
	intr->timer_last = 0;
	intr->next = NULL;

	if(_intr_list != NULL) {
		intr->next = _intr_list;
	}
	_intr_list = intr;
	return 0;
}

static int timer_dcntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)ret;
	(void)p;

	if(cmd == 0) { 
		uint32_t usec = (uint32_t)proto_read_int(in);
		uint32_t entry = (uint32_t)proto_read_int(in);
		interrupt_setup(from_pid, usec, entry);
	}	
	return 0;
}

static void interrupt_handle(uint32_t interrupt) {
	(void)interrupt;
	uint64_t usec;
	kernel_tic(NULL, &usec);
	interrupt_t* intr = _intr_list;
	while(intr != NULL) {
		if(intr->timer_last == 0)
			intr->timer_last = usec;
		else if((usec - intr->timer_last) >= intr->timer_usec) {
			intr->timer_last = usec;
			syscall2(SYS_SOFT_INT, intr->pid, intr->entry);
		}
		intr = intr->next;
	}
	sys_interrupt_end();
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/timer";
	_intr_list = NULL;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "timer");
	dev.dev_cntl = timer_dcntl;

	sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle);
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
