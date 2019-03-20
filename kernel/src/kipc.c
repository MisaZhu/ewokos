#include <kipc.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kalloc.h>
#include <system.h>
#include <dev/uart.h>
#include <printk.h>
#include <mm/shm.h>

typedef struct channel {
	uint32_t offset;
	uint32_t size;
	int32_t shmID;
	uint32_t shmSize;

	struct {
		int32_t pid; //proc id
		void* shm; //sharemem
	} procMap[2]; // item 0 also means owner(creator).

	int32_t ring; //ready for procMap[0].pid or procMap[1].pid, or 0 if closed. 
} channel_t;

#define CHANNEL_MAX 128

static channel_t _channels[CHANNEL_MAX];

void ipc_init() {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		memset(&_channels[i], 0, sizeof(channel_t));
		_channels[i].ring = -1;
		_channels[i].shmID = -1;
	}
}

/*open ipcernel ipc channel*/
int32_t ipc_open(int32_t pid, uint32_t buf_size) {
	int32_t ret = -1;
	if(buf_size < PAGE_SIZE)
		buf_size = PAGE_SIZE;

	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		if(_channels[i].ring < 0) {
			int32_t id = shm_alloc(buf_size); 
			if(id < 0) {
				printk("Panic: shm_alloc error when open ipcchannel!\n");
				break;
			}
			_channels[i].shmID = id;
			_channels[i].shmSize = buf_size;
			_channels[i].ring = 0;
			_channels[i].procMap[0].pid = _current_proc->pid;
			_channels[i].procMap[0].shm = shm_proc_map(_current_proc->pid, id);
			_channels[i].procMap[1].pid = pid;
			_channels[i].procMap[1].shm = shm_proc_map(pid, id);
			proc_wake(_channels[i].procMap[0].pid);
			proc_wake(_channels[i].procMap[1].pid);
			ret = i;
			break;
		}
	}

	if(ret < 0)
		printk("Panic: ipcchannels are all busy!\n");
	return ret;
}

static inline channel_t* ipc_get_channel(int32_t id) {
	if(id < 0 || id >= CHANNEL_MAX || _channels[id].ring < 0)
		return NULL;
	return &_channels[id];
}

static inline bool check_channel(channel_t* channel) {
	if(channel == NULL)
		return false;

	int32_t pid = _current_proc->pid;
	return (channel->procMap[1].pid == pid || channel->procMap[0].pid == pid);
}

/*close kernel ipc channel*/
int32_t ipc_close(int32_t id) {
	channel_t* channel = ipc_get_channel(id);
	if(!check_channel(channel)) {
		return 0;
	}

	if(channel->size > 0) //still got buffer data to read.
		return -1;

	shm_proc_unmap(channel->procMap[0].pid, channel->shmID);
	shm_proc_unmap(channel->procMap[1].pid, channel->shmID);
	shm_free(channel->shmID);

	proc_wake(channel->procMap[0].pid);
	proc_wake(channel->procMap[1].pid);

	memset(channel, 0, sizeof(channel_t));
	channel->ring = -1;
	channel->shmID = -1;
	return 0;
}

int ipc_ring(int32_t id) {
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL)
		return -1;
	
	if(channel->procMap[channel->ring].pid != _current_proc->pid) //current proc dosen't hold the ring.
		return 0;

	if(channel->ring == 0)
		channel->ring = 1;
	else 
		channel->ring = 0;

	proc_wake(channel->procMap[channel->ring].pid);
	channel->offset = 0;
	return 0;
}

static int peerChannel(channel_t* channel, int pid) {
	if(channel->procMap[0].pid == pid)
		return channel->procMap[1].pid;
	return  channel->procMap[0].pid;
}

int ipc_peer(int32_t id) {
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL)
		return -1;
	return peerChannel(channel, _current_proc->pid);
}

int ipc_write(int32_t id, void* data, uint32_t size) {
	channel_t* channel = ipc_get_channel(id);
	if(size == 0)
		return 0; 
	if(channel->ring < 0 || channel->shmID < 0)  //closed.
		return 0;
		
	int32_t pid = _current_proc->pid;
	if(channel->procMap[channel->ring].pid != pid) {//not read for current proc.
		proc_sleep(pid);
		return -1;
	}

	if((size + channel->offset) > channel->shmSize)
		size = PAGE_SIZE - channel->offset;

	char* p = (char*)channel->procMap[channel->ring].shm;
	if(p == NULL || size == 0) {//if channel full.
		//proc_wake(peerChannel(channel, pid));
		return 0;
	}

	p = p + channel->offset;
	memcpy(p, data, size);
	channel->offset += size;
	channel->size = channel->offset;
	//proc_wake(peerChannel(channel, pid));
	return size;
}

int32_t ipc_read(int32_t id, void* data, uint32_t size) {
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL || data == NULL || size == 0)
		return 0;

	if(channel->ring < 0 || channel->shmID < 0)  //closed.
		return 0;
	
	int32_t pid = _current_proc->pid;
	if(channel->procMap[channel->ring].pid != pid) {//not read for current proc.
		proc_sleep(pid);
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

int32_t ipc_ready() {
	int32_t pid = _current_proc->pid;
	
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		channel_t* channel = &_channels[i];
		if(channel->procMap[channel->ring].pid == pid) // current process hold the ring 
			break;
	}

	if(i >= CHANNEL_MAX) { // not channel ring for current proc
		proc_sleep(pid);
		return -1;
	}
	return i;
}

/*close all channels of specific process*/
void ipc_close_all(int32_t pid) {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		channel_t* channel = &_channels[i];
		if(channel->ring < 0)
			continue;
		if(channel->procMap[0].pid == pid || channel->procMap[1].pid == pid) 
			ipc_close(i);
	}
}
