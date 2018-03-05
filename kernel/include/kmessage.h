#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <types.h>
#include <package.h>

typedef struct KMessage {
	struct KMessage* next;
	struct KMessage* prev;

	PackageT *pkg;	
} KMessageT;

typedef struct {
	KMessageT* head;
	KMessageT* tail;
} MessageQueueT;

int ksend(int id, int pid, PackageT* pkg);

/*read message , if id < 0 means read the first one*/
PackageT* krecv(int id);

void clearMessageQueue(MessageQueueT* queue);

#endif
