#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/timer.h>
#include <ewoksys/syscall.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/kernel_tic.h>

typedef struct interrupt_st {
	uint32_t id;
	uint32_t proc_uuid;
	int32_t  pid;
	uint32_t entry;
	uint32_t data;
	uint32_t timer_usec;
	uint64_t timer_last;
	struct   interrupt_st* next;
} interrupt_t;
static interrupt_t* _intr_list = NULL;
static uint32_t _id = 0;

static int32_t interrupt_remove(int32_t pid, uint32_t id) {
	interrupt_t* p = _intr_list;
	interrupt_t* prev = NULL;
	while(p != NULL) {
		interrupt_t* next = p->next;
		if(p->pid == pid) {
			if(id == 0 || p->id == id) {
				if(p == _intr_list)	
					_intr_list = next;
				if(prev != NULL)
					prev->next = next;
				free(p);
				p = NULL;
			}
		}

		if(p != NULL)
			prev = p;
		p = next;
	}
	return 0;
}

static int32_t interrupt_setup(int32_t pid, uint32_t timer_usec, uint32_t entry, uint32_t data) {
	interrupt_t* intr = (interrupt_t*)malloc(sizeof(interrupt_t));
	intr->pid = pid;
	intr->proc_uuid = syscall1(SYS_PROC_UUID, pid);
	intr->entry = entry;
	intr->data = data;
	intr->timer_usec = timer_usec;
	intr->timer_last = 0;
	intr->next = NULL;
	intr->id = _id++;

	if(_intr_list != NULL) {
		intr->next = _intr_list;
	}
	_intr_list = intr;
	return intr->id;
}

static void interrupt_handle(uint32_t interrupt, uint32_t data) {
	(void)interrupt;
	(void)data;
	uint64_t usec;
	//ipc_disable();

	kernel_tic(NULL, &usec);
	interrupt_t* intr = _intr_list;
	interrupt_t* prev = NULL;
	while(intr != NULL) {
		interrupt_t* next = intr->next;
		if(proc_check_uuid(intr->pid, intr->proc_uuid) == intr->proc_uuid) {
			if(intr->timer_last == 0)
				intr->timer_last = usec;
			else if((usec - intr->timer_last) >= intr->timer_usec) {
				intr->timer_last = usec;
				syscall3(SYS_SOFT_INT, intr->pid, intr->entry, intr->data);
			}
			prev = intr;
		}
		else { //remove unavailable proc
			if(intr == _intr_list)
				_intr_list = next;

			free(intr);
			intr = NULL;

			if(prev != NULL)
				prev->next = next;
		}
		intr = next;
	}

	if(_intr_list == NULL)
		sys_interrupt_setup(SYS_INT_TIMER0, 0, 0);

	//ipc_enable();
	sys_interrupt_end();
}

static int timer_dcntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)p;

	if(cmd == TIMER_SET) { 
		if(_intr_list == NULL)
			sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle, 0);
		uint32_t usec = (uint32_t)proto_read_int(in);
		uint32_t entry = (uint32_t)proto_read_int(in);
		uint32_t data = (uint32_t)proto_read_int(in);
		uint32_t id = interrupt_setup(from_pid, usec, entry, data);
		PF->addi(ret, id);
	}	
	else if(cmd == TIMER_REMOVE) { 
		uint32_t id = (uint32_t)proto_read_int(in);
		interrupt_remove(from_pid, id);
		if(_intr_list == NULL)
			sys_interrupt_setup(SYS_INT_TIMER0, 0, 0);
	}
	return 0;
}


int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/timer";
	_intr_list = NULL;
	_id = 1;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "timer");
	dev.dev_cntl = timer_dcntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
