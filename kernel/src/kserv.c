#include <kserv.h>
#include <kalloc.h>
#include <lib/string.h>
#include <lib/error.h>

#define KS_BUFFER_IDLE 	0
#define KS_BUFFER_REQ 	1	
#define KS_BUFFER_RESP	2

#define KSERV_RETRY_MAX 1024

typedef struct {
	void* buffer;
	uint32_t retry;
	uint32_t size : 30;
	uint32_t state : 2;
	int owner; //owner process id
	int client; //client process id
} KServT;

static KServT _ksTable[] = {
	[KS_FS] = {0, 0 , 0, 0, -1, -1},
	[KS_END] = {0, 0, 0, 0, -1, -1},
};


bool kservReg(KSNameT name) {
	if(name >= KS_END) //no such service.
		return false;
	if(_ksTable[name].owner >= 0) //already be registered
		return false;

	_ksTable[name].buffer = kalloc1k();
	_ksTable[name].owner = _currentProcess->pid;
	return true;
}

void kservUnreg(ProcessT* proc) {
	for(int i=0; i<KS_END; i++) {
		if(_ksTable[i].owner == proc->pid) {
			kfree1k(_ksTable[i].buffer);
			_ksTable[i].owner = -1;
			_ksTable[i].client = -1;
			_ksTable[i].size = 0;
			_ksTable[i].retry = 0;
			_ksTable[i].buffer = 0;
		}
	}	
}

#define KS_BUFFER_SIZE 1024 //1k

int kservRequest(KSNameT name, char* p, uint32_t size) {
	if(_ksTable[name].owner < 0 || 
			_ksTable[name].buffer == 0) // kservice not exist.
		return E_ERROR;

	if(_ksTable[name].state != KS_BUFFER_IDLE &&
			_ksTable[name].retry < KSERV_RETRY_MAX) {
		// server still busy on other task. need to retry later.
		_ksTable[name].retry++;
		return E_RETRY;
	}

	_ksTable[name].retry = 0;
	
	if(size > KS_BUFFER_SIZE)
		size = KS_BUFFER_SIZE;
	
	memcpy(_ksTable[name].buffer, p, size);
	_ksTable[name].size = size;
	_ksTable[name].state = KS_BUFFER_REQ; 
	_ksTable[name].client = _currentProcess->pid;
	return size;
}

int kservResponse(KSNameT name, char* p, uint32_t size) {
	if(_ksTable[name].owner != _currentProcess->pid || 
			_ksTable[name].client < 0 ||
			_ksTable[name].buffer == 0) // kservice not right!
		return E_ERROR;

	if(_ksTable[name].state != KS_BUFFER_REQ &&
			_ksTable[name].retry < KSERV_RETRY_MAX) {
		_ksTable[name].retry++;
		return E_RETRY;
	}

	if(_ksTable[name].retry >= KSERV_RETRY_MAX) {
		_ksTable[name].state = KS_BUFFER_IDLE;
		_ksTable[name].retry = 0;
		_ksTable[name].size = 0;
		_ksTable[name].client = -1;
		return E_ERROR;
	}	

	if(size > KS_BUFFER_SIZE)
		size = KS_BUFFER_SIZE;
	
	memcpy(_ksTable[name].buffer, p, size);
	_ksTable[name].size = size;
	_ksTable[name].state = KS_BUFFER_RESP;
	return size;
}

int kservGetResponse(KSNameT name, char* p, uint32_t size) {
	if(_ksTable[name].owner < 0 ||
			_ksTable[name].client < 0 ||
			_ksTable[name].buffer == 0) { // request not exist.
		return E_ERROR;
	}

	if(_ksTable[name].client != _currentProcess->pid ||
			_ksTable[name].state != KS_BUFFER_RESP) { //not for you or not response!
		return E_RETRY;
	}

	/*!!Notice: if size < buffer data size, will drop the rest*/
	if(size > _ksTable[name].size)
		size = _ksTable[name].size;
	memcpy(p, _ksTable[name].buffer, size);
	_ksTable[name].size = 0;
	_ksTable[name].state = KS_BUFFER_IDLE;
	_ksTable[name].client = -1;
	return size;
}

//server read request from client
int kservGetRequest(KSNameT name, int *client, char* p, uint32_t size) {
	if(_ksTable[name].owner != _currentProcess->pid ||
			_ksTable[name].buffer == 0) { // kservice not exist.
		return E_ERROR;
	}

	if(_ksTable[name].client < 0 ||
			_ksTable[name].state != KS_BUFFER_REQ) { //no request yet. need to retry later.
		return E_RETRY;	
	}

	*client = _ksTable[name].client;
	/*!!Notice: if size < buffer data size, will drop the rest*/
	if(size > _ksTable[name].size)
		size = _ksTable[name].size;
	memcpy(p, _ksTable[name].buffer, size);
	_ksTable[name].size = 0;
	return size;
}

