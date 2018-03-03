#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>
#include <pmessage.h>

typedef struct Message {
	struct Message* next;
	struct Message* prev;

	int fromPid; /*source pid*/
	int32_t size;
	void* data;
} MessageT;

typedef struct {
	MessageT* head;
	MessageT* tail;
} MessageQueueT;

bool ksendMessage(int toPid, void* data, int32_t size);

/*read message from fromPid process, if fromPid < 0 means from any process*/
ProcMessageT* kreadMessage(int fromPid);

void clearMessageQueue(MessageQueueT* queue);

#endif
