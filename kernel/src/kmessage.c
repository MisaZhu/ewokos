#include <kmessage.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kalloc.h>
#include <system.h>
#include <dev/uart.h>

typedef struct Channel {
	uint32_t status;
	uint32_t size;
	uint32_t offset;
	void* buffer;

	int32_t readPid;
	int32_t writePid;
} ChannelT;

#define CHANNEL_MAX 128

enum {
	CHANNEL_FREE = 0,
	CHANNEL_IDLE = 1,
	CHANNEL_READ = 2,
	CHANNEL_WRITE = 3
};

static ChannelT _channels[CHANNEL_MAX];

void kinit() {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		memset(&_channels[i], 0, sizeof(ChannelT));
	}
}

/*open kernel ipc channel*/
int32_t kopen(int32_t pid) {
	int32_t ret = -1;

	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		if(_channels[i].status == CHANNEL_FREE) {
			if(_channels[i].buffer == NULL) {
				void* buffer = kalloc(); 
				if(buffer == NULL) {
					uartPuts("panic: kalloc error when open kchannel!\n");
					break;
				}
				_channels[i].buffer = buffer;
			}
			_channels[i].writePid = _currentProcess->pid;
			_channels[i].readPid = pid;
			_channels[i].status = CHANNEL_IDLE;
			_channels[i].offset = 0;
			ret = i;
			break;
		}
	}

	if(ret < 0)
		uartPuts("panic: kchannels are all busy!\n");
	return ret;
}

static inline ChannelT* kgetChannel(int32_t id) {
	if(id < 0 || id >= CHANNEL_MAX || _channels[id].status == CHANNEL_FREE) 
		return NULL;
	return &_channels[id];
}

static inline bool checkChannel(ChannelT* channel) {
	if(channel == NULL)
		return false;

	int32_t pid = _currentProcess->pid;
	return (channel->writePid == pid || channel->readPid == pid);
}

/*close kernel ipc channel*/
void kclose(int32_t id) {
	ChannelT* channel = kgetChannel(id);
	if(!checkChannel(channel)) {
		return;
	}

	void* p = channel->buffer;
	memset(channel, 0, sizeof(ChannelT));
	channel->buffer = p;
}

int kwrite(int32_t id, void* data, uint32_t size) {
	ChannelT* channel = kgetChannel(id);
	//if(!checkChannel(channel) || data == NULL) 
	//	return 0;
	if(size == 0 || channel->status == CHANNEL_FREE) //if closed.
		return 0; 

	if(channel->status != CHANNEL_IDLE) //not read for writing.
		return -1;

	channel->status = CHANNEL_WRITE; //set channel busy in writing.
	if(size > PAGE_SIZE)
		size = PAGE_SIZE;
	memcpy(channel->buffer, data, size);
	channel->size = size;
	channel->offset = 0;
	channel->status = CHANNEL_READ; //set channel waiting to be read.
	
	int32_t pid = _currentProcess->pid;
	if(channel->readPid == pid) { //swap to/from pid
		int32_t tmp = channel->writePid;	
		channel->writePid = pid;
		channel->readPid = tmp;
	}

	return size;
}

int32_t kread(int32_t id, void* data, uint32_t size) {
	ChannelT* channel = kgetChannel(id);
	if(channel == NULL || data == NULL)
		return 0;
	if(size == 0 || channel->status == CHANNEL_FREE) //if closed.
		return 0;
	
	int32_t pid = _currentProcess->pid;

	if(channel->status != CHANNEL_READ || channel->readPid != pid) //waiting if not ready to read.
		return -1;

	const char* p = ((const char*)(channel->buffer)) + channel->offset;
	if(size >= channel->size) {
		memcpy(data, p, channel->size);
		size = channel->size;
		channel->status = CHANNEL_IDLE; //set channel idle.
		channel->offset = 0;
		channel->size = 0;
	}
	else { //keep read status for rest.
		memcpy(data, p, size);
		channel->offset += size;
		channel->size -= size;
	}

	return size;
}

int32_t kgetPidW(int32_t id) {
	ChannelT* channel = kgetChannel(id);
	if(channel == NULL)
		return -1;
	return channel->writePid;
}
	
int32_t kgetPidR(int32_t id) {
	ChannelT* channel = kgetChannel(id);
	if(channel == NULL)
		return -1;
	return channel->readPid;
}

int32_t kready() {
	int32_t pid = _currentProcess->pid;
	
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		ChannelT* channel = &_channels[i];
		if(channel->status != CHANNEL_READ)
			continue;
		if(channel->readPid == pid)
			break;
	}

	if(i >= CHANNEL_MAX)
		return -1;
	return i;
}

void kcloseAll() {
	int32_t pid = _currentProcess->pid;
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		ChannelT* channel = &_channels[i];
		if(channel->status == CHANNEL_FREE)
			continue;
		if(channel->readPid == pid || channel->writePid == pid) 
			kclose(i);
	}
}
