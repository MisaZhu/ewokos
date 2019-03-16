#include <kipc.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kalloc.h>
#include <system.h>
#include <dev/uart.h>
#include <printk.h>
#include <mm/shm.h>

typedef struct Channel {
	uint32_t offset;
	uint32_t size;
	int32_t shmID;
	uint32_t shmSize;

	struct {
		int32_t pid; //proc id
		void* shm; //sharemem
	} procMap[2]; // item 0 also means owner(creator).

	int32_t ring; //ready for procMap[0].pid or procMap[1].pid, or 0 if closed. 
} ChannelT;

#define CHANNEL_MAX 128

static ChannelT _channels[CHANNEL_MAX];

void ipcInit() {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		memset(&_channels[i], 0, sizeof(ChannelT));
		_channels[i].ring = -1;
		_channels[i].shmID = -1;
	}
}

/*open ipcernel ipc channel*/
int32_t ipcOpen(int32_t pid, uint32_t bufSize) {
	int32_t ret = -1;
	if(bufSize < PAGE_SIZE)
		bufSize = PAGE_SIZE;

	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		if(_channels[i].ring < 0) {
			int32_t id = shmalloc(bufSize); 
			if(id < 0) {
				printk("Panic: shmalloc error when open ipcchannel!\n");
				break;
			}
			_channels[i].shmID = id;
			_channels[i].shmSize = bufSize;
			_channels[i].ring = 0;
			_channels[i].procMap[0].pid = _currentProcess->pid;
			_channels[i].procMap[0].shm = shmProcMap(_currentProcess->pid, id);
			_channels[i].procMap[1].pid = pid;
			_channels[i].procMap[1].shm = shmProcMap(pid, id);
			procWake(_channels[i].procMap[0].pid);
			procWake(_channels[i].procMap[1].pid);
			ret = i;
			break;
		}
	}

	if(ret < 0)
		printk("Panic: ipcchannels are all busy!\n");
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
	return (channel->procMap[1].pid == pid || channel->procMap[0].pid == pid);
}

/*close kernel ipc channel*/
int32_t ipcClose(int32_t id) {
	ChannelT* channel = ipcGetChannel(id);
	if(!checkChannel(channel)) {
		return 0;
	}

	if(channel->size > 0) //still got buffer data to read.
		return -1;

	shmProcUnmap(channel->procMap[0].pid, channel->shmID);
	shmProcUnmap(channel->procMap[1].pid, channel->shmID);
	shmfree(channel->shmID);

	procWake(channel->procMap[0].pid);
	procWake(channel->procMap[1].pid);

	memset(channel, 0, sizeof(ChannelT));
	channel->ring = -1;
	channel->shmID = -1;
	return 0;
}

int ipcRing(int32_t id) {
	ChannelT* channel = ipcGetChannel(id);
	if(channel == NULL)
		return -1;
	
	if(channel->procMap[channel->ring].pid != _currentProcess->pid) //current proc dosen't hold the ring.
		return 0;

	if(channel->ring == 0)
		channel->ring = 1;
	else 
		channel->ring = 0;

	procWake(channel->procMap[channel->ring].pid);
	channel->offset = 0;
	return 0;
}

static int peerChannel(ChannelT* channel, int pid) {
	if(channel->procMap[0].pid == pid)
		return channel->procMap[1].pid;
	return  channel->procMap[0].pid;
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
	if(channel->ring < 0 || channel->shmID < 0)  //closed.
		return 0;
		
	int32_t pid = _currentProcess->pid;
	if(channel->procMap[channel->ring].pid != pid) {//not read for current proc.
		procSleep(pid);
		return -1;
	}

	if((size + channel->offset) > channel->shmSize)
		size = PAGE_SIZE - channel->offset;

	char* p = (char*)channel->procMap[channel->ring].shm;
	if(p == NULL || size == 0) {//if channel full.
		//procWake(peerChannel(channel, pid));
		return 0;
	}

	p = p + channel->offset;
	memcpy(p, data, size);
	channel->offset += size;
	channel->size = channel->offset;
	//procWake(peerChannel(channel, pid));
	return size;
}

int32_t ipcRead(int32_t id, void* data, uint32_t size) {
	ChannelT* channel = ipcGetChannel(id);
	if(channel == NULL || data == NULL || size == 0)
		return 0;

	if(channel->ring < 0 || channel->shmID < 0)  //closed.
		return 0;
	
	int32_t pid = _currentProcess->pid;
	if(channel->procMap[channel->ring].pid != pid) {//not read for current proc.
		procSleep(pid);
		return -1;
	}

	bool end = false;
	if((size + channel->offset) >= channel->size) {
		size = channel->size - channel->offset;
		end = true;
	}

	const char* p = (const char*)channel->procMap[channel->ring].shm;
	if(p == NULL)
		return 0; //closed.

	p = p + channel->offset;
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
		if(channel->procMap[channel->ring].pid == pid) // current process hold the ring 
			break;
	}

	if(i >= CHANNEL_MAX) { // not channel ring for current proc
		procSleep(pid);
		return -1;
	}
	return i;
}

/*close all channels of specific process*/
void ipcCloseAll(int32_t pid) {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		ChannelT* channel = &_channels[i];
		if(channel->ring < 0)
			continue;
		if(channel->procMap[0].pid == pid || channel->procMap[1].pid == pid) 
			ipcClose(i);
	}
}
