#ifndef PMESSAGE_H
#define PMESSAGE_H

#include <types.h>
#include <package.h>

int ipcOpen(int pid, uint32_t bufsize);

void ipcClose(int id);

int ipcSend(int id, uint32_t type, void* data, uint32_t size);

PackageT* ipcRecv(int id);

PackageT* ipcReq(int pid, uint32_t bufSize, uint32_t type, void* data, uint32_t size, bool reply);

PackageT* ipcRoll();

#endif
