#include <kserv.h>
#include <kalloc.h>
#include <lib/string.h>

typedef struct {
	void* buffer;
	uint32_t size;
	ProcessT* proc;
} KServT;

static KServT _ksTable[] = {
	[KS_FS] = {0, 0, 0},
	[KS_CONSOLE] = {0, 0, 0},
	[KS_END] = {0, 0, 0},
};


bool kservReg(KSNameT name) {
	if(name >= KS_END) //no such service.
		return false;
	if(_ksTable[name].proc != 0) //already be registered
		return false;

	_ksTable[name].buffer = kalloc1k();
	_ksTable[name].proc = _currentProcess;
	return true;
}

void kservUnreg(ProcessT* proc) {
	for(int i=0; i<KS_END; i++) {
		if(_ksTable[i].proc == proc) {
			kfree1k(_ksTable[i].buffer);
			_ksTable[i].proc = 0;
			_ksTable[i].size = 0;
			_ksTable[i].buffer = 0;
		}
	}	
}

#define KS_BUFFER_SIZE 1024 //1k

int kservWrite(KSNameT name, char* p, uint32_t size) {
	if(_ksTable[name].proc != _currentProcess || _ksTable[name].buffer == 0) //not be registered
		return -1;
	if(_ksTable[name].size > 0) //not empty. need to retry later.
		return -2;
	
	if(size > KS_BUFFER_SIZE)
		size = KS_BUFFER_SIZE;
	
	memcpy(_ksTable[name].buffer, p, size);
	_ksTable[name].size = size;
	return size;
}

int kservRead(KSNameT name, char* p, uint32_t size) {
	if(_ksTable[name].proc != _currentProcess || _ksTable[name].buffer == 0) //not be registered
		return -1;
	if(_ksTable[name].size == 0) //no data. need to retry later.
		return -2;

	/*!!Notice: if size < buffer data size, will drop the rest*/
	if(size > _ksTable[name].size)
		size = _ksTable[name].size;
	memcpy(p, _ksTable[name].buffer, size);
	_ksTable[name].size = 0;
	return size;
}


