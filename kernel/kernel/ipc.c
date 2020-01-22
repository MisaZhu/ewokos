#include <kernel/system.h>
#include <kernel/proc.h>
#include <kernel/kernel.h>
#include <kernel/schedule.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <kstring.h>

static int32_t _msg_counter = 0;

void proc_ipc_init(void) {
	_msg_counter = 0;
}

static proc_msg_t* new_msg(proc_t* proc) {
	proc_msg_t* msg = (proc_msg_t*)kmalloc(sizeof(proc_msg_t));
	if(msg == NULL) {
		return NULL;
	}
	memset(msg, 0, sizeof(proc_msg_t));
	
	if(proc->msg_queue_tail == NULL) {
		proc->msg_queue_tail = proc->msg_queue_head = msg;
	}
	else {
		proc->msg_queue_tail->next = msg;
		msg->prev = proc->msg_queue_tail;
		proc->msg_queue_tail = msg;
	}
	return msg;
}

static proc_msg_t* get_msg(proc_t* proc, int32_t id) {
	proc_msg_t* msg = proc->msg_queue_head;
	if(id < 0) {
		while(msg != NULL) {
			if(msg->type == MSG_PUB)
				return msg;
			msg = msg->next;
		}
		return NULL;
	}

	/*get msg with spec id*/
	while(msg != NULL) {
		if(msg->id == id)
			return msg;
		msg = msg->next;
	}
	return NULL;
}

static void remove_msg(proc_t* proc, proc_msg_t* msg) {
	if(msg->next != NULL)
		msg->next->prev = msg->prev;
	if(msg->prev != NULL)
		msg->prev->next = msg->next;

	if(msg == proc->msg_queue_head) 
		proc->msg_queue_head = msg->next;
	if(msg == proc->msg_queue_tail) 
		proc->msg_queue_tail = msg->prev;

	kfree(msg->data.data);
	kfree(msg);
}

proc_msg_t* proc_send_msg(int32_t to_pid, rawdata_t* data, int32_t id) {

	proc_t* proc_to = proc_get(to_pid);
	if(proc_to == NULL || proc_to->state == UNUSED) {
		return NULL;
	}

	proc_msg_t* msg = new_msg(proc_to);
	if(msg == NULL) {
		return NULL;
	}
	if(id < 0) {
		msg->id = _msg_counter++;
		msg->type = MSG_PUB;
	}
	else {
		msg->id = id;
		msg->type = MSG_ID;
	}

	msg->from_pid = _current_proc->pid;
	msg->to_pid = to_pid;
	msg->data.size = data->size;
	msg->data.data = kmalloc(data->size);
	if(msg->data.data == NULL) {
		kfree(msg);
		return NULL;
	}
	memcpy(msg->data.data, data->data, data->size);
	proc_wakeup((uint32_t)&proc_to->pid);
	return msg;
}

int32_t proc_get_msg(int32_t *pid, rawdata_t* data, int32_t id) {
	int32_t res = -1;

	proc_msg_t* msg;
	msg = get_msg(_current_proc, id);

	if(msg != NULL) {
		if(pid != NULL)
			*pid = msg->from_pid;
		data->size = msg->data.size;
		data->data = proc_malloc(msg->data.size);
		if(data->data != NULL) 
			memcpy(data->data, msg->data.data, msg->data.size);
		res = msg->id;
		remove_msg(_current_proc, msg);
	}
	return res;
}

