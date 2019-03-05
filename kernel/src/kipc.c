#include <kipc.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kalloc.h>
#include <system.h>
#include <dev/uart.h>
#include <klog.h>

typedef struct Channel {
	uint32_t offset;
	uint32_t size;
	void* buffer;

	int32_t pid1;
	int32_t pid2;
	int32_t ring; //ready for pid1 or pid2, or 0 if closed. 
} ChannelT;

#define CHANNEL_MAX 128

static ChannelT _channels[CHANNEL_MAX];

void ipcInit() {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		memset(&_channels[i], 0, sizeof(ChannelT));
		_channels[i].ring = -1;
	}
}

/*open ipcernel ipc channel*/
int32_t ipcOpen(int32_t pid) {
	int32_t ret = -1;

	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		if(_channels[i].ring < 0) {
			if(_channels[i].buffer == NULL) {
				void* buffer = kalloc(); 
				if(buffer == NULL) {
					klog("panic: ipcalloc error when open ipcchannel!\n");
					break;
				}
				_channels[i].buffer = buffer;
			}
			_channels[i].ring = _channels[i].pid1 = _currentProcess->pid;
			_channels[i].pid2 = pid;
			procWake(_channels[i].pid1);
			procWake(_channels[i].pid2);
			ret = i;
			break;
		}
	}

	if(ret < 0)
		klog("panic: ipcchannels are all busy!\n");
	return ret;
}

static inline ChannelT* ipcGetChannel(int32_t id) {
	if(id < 0 || id >= CHANNEL_MAX || _channels[id].ring < 0)
		return NULL;
	return &_channels[id];
}

static inline bool checkChannel(ChannelT* channel) {
	if(channel == NULL)
		return false;

	int32_t pid = _currentProcess->pid;
	return (channel->pid2 == pid || channel->pid1 == pid);
}

/*close kernel ipc channel*/
void ipcClose(int32_t id) {
	ChannelT* channel = ipcGetChannel(id);
	if(!checkChannel(channel)) {
		return;
	}

	procWake(channel->pid1);
	procWake(channel->pid2);

	void* p = channel->buffer;
	memset(channel, 0, sizeof(ChannelT));
	channel->buffer = p;
	channel->ring = -1;
}

int ipcRing(int32_t id) {
	ChannelT* channel = ipcGetChannel(id);
	if(channel == NULL)
		return -1;
	
	if(channel->ring != _currentProcess->pid) //current proc dosen't hold the ring.
		return 0;

	if(channel->pid1 == _currentProcess->pid)
		channel->ring = channel->pid2;
	else 
		channel->ring = channel->pid1;

	procWake(channel->ring);
	channel->offset = 0;
	return 0;
}

static int peerChannel(ChannelT* channel, int pid) {
	if(channel->pid1 == pid)
		return channel->pid2;
	return  channel->pid1;
}

int ipcPeer(int32_t id) {
	ChannelT* channel = ipcGetChannel(id);
	if(channel == NULL)
		return -1;
	return peerChannel(channel, _currentProcess->pid);
}

int ipcWrite(int32_t id, void* data, uint32_t size) {
	ChannelT* channel = ipcGetChannel(id);
	if(size == 0)
		return 0; 
	
	int32_t pid = _currentProcess->pid;
	if(channel->ring != pid) {//not read for current proc.
		if(channel->ring < 0)  //closed.
			return 0;
		procSleep(pid);
		return -1;
	}

	procWake(peerChannel(channel, pid));

	if((size + channel->offset) > PAGE_SIZE)
		size = PAGE_SIZE - channel->offset;

	if(size == 0) //if channel full.
		return 0;

	char* p = ((char*)(channel->buffer)) + channel->offset;
	memcpy(p, data, size);
	channel->offset += size;
	channel->size = channel->offset;
	return size;
}

int32_t ipcRead(int32_t id, void* data, uint32_t size) {
	ChannelT* channel = ipcGetChannel(id);
	if(channel == NULL || data == NULL || size == 0)
		return 0;
	
	int32_t pid = _currentProcess->pid;
	if(channel->ring != pid) {//not read for current proc.
		if(channel->ring < 0)  //closed.
			return 0;
		procSleep(pid);
		return -1;
	}

	bool end = false;
	if((size + channel->offset) >= channel->size) {
		size = channel->size - channel->offset;
		end = true;
	}

	const char* p = ((const char*)(channel->buffer)) + channel->offset;
	memcpy(data, p, size);
	if(end) {
		channel->offset = 0;
		channel->size = 0;
	}
	else
		channel->offset += size;
	return size;
}

int32_t ipcReady() {
	int32_t pid = _currentProcess->pid;
	
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		ChannelT* channel = &_channels[i];
		if(channel->ring == pid)
			break;
	}

	if(i >= CHANNEL_MAX) {
		procSleep(pid);
		return -1;
	}
	return i;
}

void ipcCloseAll() {
	int32_t pid = _currentProcess->pid;
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		ChannelT* channel = &_channels[i];
		if(channel->ring < 0)
			continue;
		if(channel->pid1 == pid || channel->pid2 == pid) 
			ipcClose(i);
	}
}

