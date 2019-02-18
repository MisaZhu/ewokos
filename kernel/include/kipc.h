#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>

/*init ipcernel ipc channel*/
void ipcInit();

/*open ipcernel ipc channel*/
int32_t ipcOpen(int32_t pid);

/*close ipcernel ipc channel*/
void ipcClose(int32_t id);

/*return size sent, 0 means closed, < 0 means retry. */
int32_t ipcWrite(int32_t id, void* data, uint32_t size);

/*return size read, 0 means closed, < 0 means retry. */
int32_t ipcRead(int32_t id, void* data, uint32_t size);

/*get writing pid*/
int32_t ipcGetPidW(int32_t id);

/*get reading pid*/
int32_t ipcGetPidR(int32_t id);

/*get ready to read id of current proc*/
int32_t ipcReady();

/*close all channel of current proc*/
void ipcCloseAll();

#endif
