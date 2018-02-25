#ifndef KERNEL_SERV_H
#define KERNEL_SERV_H

#include <types.h>
#include <proc.h>
#include <lib/kservtypes.h>

bool kservReg(KSNameT name);
void kservUnreg(ProcessT* proc);

/*Notice: request and getResponse must be pairly used */
//client send request to server
int kservRequest(KSNameT name, char* p, uint32_t size);
//client read response from server
int kservGetResponse(KSNameT name, char* p, uint32_t size);

/*Notice: getRequest and Response must be pairly used */
//server read request from client
int kservGetRequest(KSNameT name, int *client, char* p, uint32_t size);
//server response to client
int kservResponse(KSNameT name, char* p, uint32_t size);

#endif
