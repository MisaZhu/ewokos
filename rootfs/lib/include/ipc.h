#ifndef PMESSAGE_H
#define PMESSAGE_H

#include <types.h>
#include <package.h>

int ipc_open(int pid, uint32_t buf_size);

void ipc_close(int id);

int ipc_send(int id, uint32_t type, void* data, uint32_t size);

package_t* ipc_recv(int id);

package_t* ipc_req(int pid, uint32_t buf_size, uint32_t type, void* data, uint32_t size, bool reply);

package_t* ipc_roll();

#endif
