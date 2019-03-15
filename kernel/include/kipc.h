#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>

typedef struct IPCTask {
	int32_t to;
	int32_t from;
	uint32_t state;
	struct IPCTask* next;
	struct IPCTask* prev;
}IPCTaskT;

/*init ipcernel ipc channel*/
void ipcInit();

/*open ipcernel ipc channel*/
int32_t ipcOpen(int32_t pid, uint32_t bufSize);

/*close ipcernel ipc channel*/
int32_t ipcClose(int32_t id);

/*return size sent, 0 means closed, < 0 means retry. */
int32_t ipcWrite(int32_t id, void* data, uint32_t size);

/*return size read, 0 means closed, < 0 means retry. */
int32_t ipcRead(int32_t id, void* data, uint32_t size);

/*switch ring*/
int32_t ipcRing(int32_t id);

/*get peer pid of channel*/
int32_t ipcPeer(int32_t id);

/*get ready to read id of current proc*/
int32_t ipcReady();

/*close all channel of current proc*/
void ipcCloseAll(int32_t pid);

IPCTaskT* ipcTaskOpen(int32_t toPID);

void ipcTaskClose(IPCTaskT* task);

static inline void* ipcTaskData(IPCTaskT* task) {
	char* p = (char*)task;
	return p+sizeof(IPCTaskT);
}

#endif
