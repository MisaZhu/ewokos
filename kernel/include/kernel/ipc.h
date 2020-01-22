#ifndef IPC_H
#define IPC_H

#include <rawdata.h>

enum {
	MSG_PUB = 0,
	MSG_ID
};

typedef struct st_proc_msg {
	int32_t id;
	int32_t type;
	int32_t from_pid;
	int32_t to_pid;
	rawdata_t data;

	struct st_proc_msg *next;
	struct st_proc_msg *prev;
} proc_msg_t;


extern void proc_ipc_init(void);
extern proc_msg_t* proc_send_msg(int32_t to_pid, rawdata_t* data, int32_t id);
extern int32_t proc_get_msg(int32_t *pid, rawdata_t* data, int32_t id);

#endif
