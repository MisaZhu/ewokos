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
#include <ewoksys/mstr.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/ipc.h>

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
static uint32_t _min_timer_usec = 0;

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
	intr->proc_uuid = proc_get_uuid(pid);
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

static void update_timer_intr(void) {
	interrupt_t* intr = _intr_list;
	uint32_t min_usec = 0;
	while(intr != NULL) {
		if(min_usec == 0 || min_usec > intr->timer_usec)
			min_usec = intr->timer_usec;
		intr = intr->next;
	}

	if(min_usec != _min_timer_usec) {
		_min_timer_usec = min_usec;
	}
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
				sys_soft_intr(intr->pid, intr->entry, intr->data);
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
			update_timer_intr();
		}
		intr = next;
	}

	//if(_intr_list == NULL)
		//sys_interrupt_setup(IRQ_TIMER0, NULL);

	//ipc_enable();
}

static int timer_dcntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)p;
	from_pid = proc_getpid(from_pid);

	if(cmd == TIMER_SET) { 
		//klog("timer set\n");
		if(_intr_list == NULL) {
			static interrupt_handler_t handler;
			handler.data = 0;
			handler.handler = interrupt_handle;
			//sys_interrupt_setup(IRQ_TIMER0, &handler);
		}
		uint32_t usec = (uint32_t)proto_read_int(in);
		uint32_t entry = (uint32_t)proto_read_int(in);
		uint32_t data = (uint32_t)proto_read_int(in);
		uint32_t id = interrupt_setup(from_pid, usec, entry, data);
		PF->addi(ret, id);
		update_timer_intr();
	}	
	else if(cmd == TIMER_REMOVE) { 
		//klog("timer remove\n");
		uint32_t id = (uint32_t)proto_read_int(in);
		interrupt_remove(from_pid, id);
		update_timer_intr();
		//if(_intr_list == NULL)
			//sys_interrupt_setup(IRQ_TIMER0, NULL);
	}
	return 0;
}

static char* timer_cmd(int from_pid, int argc, char** argv, void* p) {
	if(strcmp(argv[0], "list") == 0) {
		str_t* str = str_new("pid  timer\n");
		interrupt_t* intr = _intr_list;
		while(intr != NULL) {
			char item[32];
			if(intr->timer_usec >= 1000000)
				snprintf(item, 31, "%-4d  %4d sec\n", intr->pid, intr->timer_usec/1000000);
			else if(intr->timer_usec >= 1000)
				snprintf(item, 31, "%-4d  %4d msec\n", intr->pid, intr->timer_usec/1000);
			else
				snprintf(item, 31, "%-4d  %4d usec\n", intr->pid, intr->timer_usec);
			str_add(str, item);
			intr = intr->next;
		}
		char* ret = str_detach(str);
		return ret;
	}
	return NULL;
}

static int timer_loop(void* p) {
	ipc_disable();
	interrupt_handle(0, 0);
	ipc_enable();

	if(_min_timer_usec == 0)
		proc_usleep(100000);
	else {
		proc_usleep(_min_timer_usec/2);
	}
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/timer";
	_intr_list = NULL;
	_id = 1;
	_min_timer_usec = 0;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "timer");
	dev.dev_cntl = timer_dcntl;
	dev.cmd = timer_cmd;
	dev.loop_step = timer_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
