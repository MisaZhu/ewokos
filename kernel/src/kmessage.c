#include <kmessage.h>
#include <kmalloc.h>
#include <string.h>
#include <proc.h>

bool ksendMessage(int toPid, void* data, int32_t size) {
	if(size == 0 || data == NULL) {
		return false;
	}

	ProcessT* toProc = procGet(toPid);
	if(toProc == NULL) { 
		return false;
	}

	MessageT* msg = kmalloc(size + sizeof(MessageT));
	if(msg == NULL) {
		return false;
	}

	msg->fromPid = _currentProcess->pid;
	msg->size = size;
	msg->next = NULL;
	msg->prev = NULL;
	msg->data = ((char*)msg) + sizeof(MessageT);

	memcpy(msg->data, data, size);	

	if(toProc->messageQueue.head == NULL) {
		toProc->messageQueue.head =	toProc->messageQueue.tail = msg;	
	}
	else {
		toProc->messageQueue.tail->next = msg;	
		msg->prev = toProc->messageQueue.tail;
		toProc->messageQueue.tail = msg;	
	}

	return true;
}

ProcMessageT* kreadMessage(int fromPid) {
	MessageQueueT* queue = &(_currentProcess->messageQueue);
	MessageT* msg = queue->head;

	while(msg != NULL) {
		if(fromPid < 0 || fromPid == msg->fromPid)
			break;
		msg = msg->next;
	}

	if(msg == NULL)
		return NULL;
	
	ProcMessageT* ret = (ProcMessageT*)pmalloc(&_currentProcess->mallocMan,
			sizeof(ProcMessageT) + msg->size);
	if(ret == NULL)
		return NULL;

	ret->data = ((char*)ret) + sizeof(ProcMessageT);
	memcpy(ret->data, msg->data, msg->size);
	ret->fromPid = msg->fromPid;
	ret->size = msg->size;

	/*free message in queue*/
	if(msg->prev != NULL)
		msg->prev->next = msg->next;
	else 
		queue->head = msg->next;

	if(msg->next != NULL)
		msg->next->prev = msg->prev;
	else
		queue->tail = msg->prev;

	kmfree(msg);
	return ret;
}

void clearMessageQueue(MessageQueueT* queue) {
	MessageT* msg = queue->head;
	while(msg != NULL) {
		MessageT* fr = msg;
		msg = msg->next;
		kmfree(fr);
	}
	queue->head = 0;
	queue->tail = 0;
}

