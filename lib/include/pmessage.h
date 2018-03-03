#ifndef PMESSAGE_H
#define PMESSAGE_H

typedef struct {
	int fromPid;
	void* data;	
	int size;
} ProcMessageT;

int psendMessage(int toPid, void* data, int size);

ProcMessageT* preadMessage(int fromPid);

#endif
