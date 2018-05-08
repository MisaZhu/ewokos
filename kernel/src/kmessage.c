#include <kmessage.h>
#include <string.h>
#include <proc.h>
#include <mm/kmalloc.h>

static int _pkgIDCount = 0;

int ksend(int id, int pid, PackageT* pkg) {
	if(pkg == NULL) {
		return -1;
	}

	ProcessT* toProc = procGet(pid);
	if(toProc == NULL) { 
		return -1;
	}

	KMessageT* msg = kmalloc(sizeof(KMessageT));
	if(msg == NULL) {
		return -1;
	}

	uint32_t pkgSize = getPackageSize(pkg);
	msg->pkg = (PackageT*)kmalloc(pkgSize);
	if(msg->pkg == NULL) {
		kmfree(msg);
		return -1;
	}	

	if(id < 0)
		pkg->id = _pkgIDCount++;
	else
		pkg->id = id;
	pkg->pid = _currentProcess->pid;
	memcpy(msg->pkg, pkg, pkgSize);	

	msg->next = NULL;
	msg->prev = NULL;

	if(toProc->messageQueue.head == NULL) {
		toProc->messageQueue.head =	toProc->messageQueue.tail = msg;	
	}
	else {
		toProc->messageQueue.tail->next = msg;	
		msg->prev = toProc->messageQueue.tail;
		toProc->messageQueue.tail = msg;	
	}

	return pkg->id;
}

PackageT* krecv(int id) {
	MessageQueueT* queue = &(_currentProcess->messageQueue);
	KMessageT* msg = queue->head;

	while(msg != NULL) {
		if(id < 0 || id == msg->pkg->id)
			break;
		msg = msg->next;
	}

	if(msg == NULL)
		return NULL;

	uint32_t pkgSize = getPackageSize(msg->pkg);
	
	PackageT* ret = (PackageT*)trunkMalloc(&_currentProcess->mallocMan, pkgSize);
	if(ret == NULL)
		return NULL;

	memcpy(ret, msg->pkg, pkgSize);	

	/*free message in queue*/
	if(msg->prev != NULL)
		msg->prev->next = msg->next;
	else 
		queue->head = msg->next;

	if(msg->next != NULL)
		msg->next->prev = msg->prev;
	else
		queue->tail = msg->prev;

	
	kmfree(msg->pkg);
	kmfree(msg);
	return ret;
}

void clearMessageQueue(MessageQueueT* queue) {
	KMessageT* msg = queue->head;
	while(msg != NULL) {
		KMessageT* fr = msg;
		msg = msg->next;
		kmfree(fr->pkg);
		kmfree(fr);
	}
	queue->head = 0;
	queue->tail = 0;
}

