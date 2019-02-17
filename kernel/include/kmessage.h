#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>

/*init kernel ipc channel*/
void kinit();

/*open kernel ipc channel*/
int32_t kopen(int32_t pid);

/*close kernel ipc channel*/
void kclose(int32_t id);

/*return size sent, 0 means closed, < 0 means retry. */
int32_t kwrite(int32_t id, void* data, uint32_t size);

/*return size read, 0 means closed, < 0 means retry. */
int32_t kread(int32_t id, void* data, uint32_t size);

/*get writing pid*/
int32_t kgetPidW(int32_t id);

/*get reading pid*/
int32_t kgetPidR(int32_t id);

/*get ready to read id of current proc*/
int32_t kready();

/*close all channel of current proc*/
void kcloseAll();

#endif
