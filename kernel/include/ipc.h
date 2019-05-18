#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>

/*init ipcernel ipc channel*/
void ipc_init(void);

/*open ipcernel ipc channel*/
int32_t ipc_open(int32_t pid, uint32_t buf_size);

/*close ipcernel ipc channel*/
int32_t ipc_close(int32_t id);

/*return size sent, 0 means closed, < 0 means retry. */
int32_t ipc_write(int32_t id, void* data, uint32_t size);

/*return size read, 0 means closed, < 0 means retry. */
int32_t ipc_read(int32_t id, void* data, uint32_t size);

/*switch ring*/
int32_t ipc_ring(int32_t id);

/*get peer pid of channel*/
int32_t ipc_peer(int32_t id);

/*get ready to read id of current proc*/
int32_t ipc_ready(bool block);

/*close all channel of current proc*/
void ipc_close_all(int32_t pid);

#endif
