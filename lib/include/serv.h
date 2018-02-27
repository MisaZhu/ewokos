#ifndef SERV_H
#define SERV_H

#include <types.h>
#include <servtypes.h>

int servReg(int serv);

//for server.
int servGetRequestA(int serv, int* client, char* p, int size);
int servResponseA(int serv, char* p, int size);
int servGetRequest(int serv, int* client, char* p, int size); //block mode
int servResponse(int serv, char* p, int size); //block mode

//for client.
int servRequestA(int serv, char* p, int size);
int servGetResponseA(int serv, char* p, int size);
int servRequest(int serv, char* p, int size); //block mode
int servGetResponse(int serv, char* p, int size); //block mode

#endif
