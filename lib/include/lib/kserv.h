#ifndef KSERV_H
#define KSERV_H

#include <types.h>
#include <lib/kservtypes.h>

int kservReg(int serv);

//for server.
int kservGetRequestA(int serv, int* client, char* p, int size);
int kservResponseA(int serv, char* p, int size);
int kservGetRequest(int serv, int* client, char* p, int size); //block mode
int kservResponse(int serv, char* p, int size); //block mode

//for client.
int kservRequestA(int serv, char* p, int size);
int kservGetResponseA(int serv, char* p, int size);
int kservRequest(int serv, char* p, int size); //block mode
int kservGetResponse(int serv, char* p, int size); //block mode

#endif
