#include <ipc.h>
#include <kstring.h>
#include <proc.h>
#include <mm/kalloc.h>
#include <system.h>
#include <printk.h>
#include <mm/shm.h>

typedef struct channel {
	uint32_t offset;
	uint32_t size;
	int32_t shm_id;
	uint32_t shm_size;
	uint32_t retry_times;

	struct {
		int32_t pid; //proc id
		void* shm; //sharemem
	} proc_map[2]; // item 0 also means owner(creator).

	int32_t ring; //ready for proc_map[0].pid or proc_map[1].pid, or 0 if closed. 
} channel_t;

#define CHANNEL_MAX 128
#define RETRY_MAX 1024

static channel_t _channels[CHANNEL_MAX];

void ipc_init() {
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		memset(&_channels[i], 0, sizeof(channel_t));
		_channels[i].ring = -1;
		_channels[i].shm_id = -1;
	}
}

static int32_t _p_lock = 0;

/*open ipcernel ipc channel*/
int32_t ipc_open(int32_t pid, uint32_t buf_size) {
	CRIT_IN(_p_lock)
	if(buf_size < PAGE_SIZE)
		buf_size = PAGE_SIZE;

	int32_t id = shm_alloc(buf_size); 
	if(id < 0) {
		printk("Panic: shm_alloc error when open ipcchannel!\n");
		CRIT_OUT(_p_lock)
		return -1;
	}

	int32_t ret = -1;
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		if(_channels[i].ring < 0) {
			_channels[i].shm_id = id;
			_channels[i].shm_size = buf_size;
			_channels[i].ring = 0;
			_channels[i].retry_times = 0;
			_channels[i].proc_map[0].pid = _current_proc->pid;
			_channels[i].proc_map[0].shm = shm_proc_map(_current_proc->pid, id);
			_channels[i].proc_map[1].pid = pid;
			_channels[i].proc_map[1].shm = shm_proc_map(pid, id);
			ret = i;
			break;
		}
	}
	if(ret < 0) {
		shm_free(id);
		printk("Panic: ipcchannels are all busy!\n");
	}
	CRIT_OUT(_p_lock)
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
	return (channel->proc_map[1].pid == pid || channel->proc_map[0].pid == pid);
}

/*close kernel ipc channel*/
static int32_t ipc_close_raw(int32_t pid, channel_t* channel) {
	shm_proc_unmap(channel->proc_map[0].pid, channel->shm_id);
	shm_proc_unmap(channel->proc_map[1].pid, channel->shm_id);
	shm_free(channel->shm_id);

	if(pid == channel->proc_map[0].pid)
		proc_wake(channel->proc_map[1].pid);
	else
		proc_wake(channel->proc_map[1].pid);
	memset(channel, 0, sizeof(channel_t));
	channel->ring = -1;
	channel->shm_id = -1;
	return 0;
}

/*close kernel ipc channel, 
return -1 means  still got buffer data to read.
*/
int32_t ipc_close(int32_t id) {
	CRIT_IN(_p_lock)
	channel_t* channel = ipc_get_channel(id);
	int32_t ret = -1;
	if(check_channel(channel)) {
		if(channel->size == 0)
			ret = ipc_close_raw(_current_proc->pid, channel);
	}	
	else {
		ret = 0;
	}
	CRIT_OUT(_p_lock)
	return ret;
}

int32_t ipc_ring(int32_t id) {
	CRIT_IN(_p_lock)
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL) {
		CRIT_OUT(_p_lock)
		return -1;
	}
	if(channel->proc_map[channel->ring].pid != _current_proc->pid) {//current proc dosen't hold the ring.
		CRIT_OUT(_p_lock)
		return 0;
	}
	if(channel->ring == 0)
		channel->ring = 1;
	else 
		channel->ring = 0;

	proc_wake(channel->proc_map[channel->ring].pid);
	channel->offset = 0;
	CRIT_OUT(_p_lock)
	return 0;
}

static int32_t peer_channel(channel_t* channel, int32_t pid) {
	if(channel->proc_map[0].pid == pid)
		return channel->proc_map[1].pid;
	return  channel->proc_map[0].pid;
}

int32_t ipc_peer(int32_t id) {
	CRIT_IN(_p_lock)
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL) {
		CRIT_OUT(_p_lock)
		return -1;
	}
	int32_t ret = peer_channel(channel, _current_proc->pid);
	CRIT_OUT(_p_lock)
	return ret;
}

int32_t ipc_write(int32_t id, void* data, uint32_t size) {
	CRIT_IN(_p_lock)
	int32_t pid = _current_proc->pid;
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL || size == 0 || channel->ring < 0 || channel->shm_id < 0) { //closed.
		channel->retry_times = 0;
		CRIT_OUT(_p_lock)
		return 0;
	}
	if(channel->proc_map[channel->ring].pid != pid) {//not ready for current proc.
		channel->retry_times++;
		CRIT_OUT(_p_lock)
		if(channel->retry_times >= RETRY_MAX)
			return 0; //close
		return -1; //retry
	}
	channel->retry_times = 0;

	if((size + channel->offset) > channel->shm_size)
		size = PAGE_SIZE - channel->offset;

	char* p = (char*)channel->proc_map[channel->ring].shm;
	if(p == NULL || size == 0) {//if channel full.
		CRIT_OUT(_p_lock)
		return 0;
	}

	p = p + channel->offset;
	memcpy(p, data, size);
	channel->offset += size;
	channel->size = channel->offset;
	CRIT_OUT(_p_lock)
	return size;
}

int32_t ipc_read(int32_t id, void* data, uint32_t size) {
	CRIT_IN(_p_lock)
	channel_t* channel = ipc_get_channel(id);
	if(channel == NULL || data == NULL || size == 0 || 
			channel->ring < 0 ||channel->shm_id < 0) { //closed.
		channel->retry_times = 0;
		CRIT_OUT(_p_lock)
		return 0;
	}
	
	int32_t pid = _current_proc->pid;
	if(channel->proc_map[channel->ring].pid != pid) {//not read for current proc.
		channel->retry_times++;
		CRIT_OUT(_p_lock)
		if(channel->retry_times >= RETRY_MAX)
			return 0; //close
		return -1; //retry.
	}
	channel->retry_times = 0;

	bool end = false;
	if((size + channel->offset) >= channel->size) {
		size = channel->size - channel->offset;
		end = true;
	}

	const char* p = (const char*)channel->proc_map[channel->ring].shm;
	if(p == NULL) {
		CRIT_OUT(_p_lock)
		return 0; //closed.
	}

	p = p + channel->offset;
	memcpy(data, p, size);
	if(end) {
		channel->offset = 0;
		channel->size = 0;
	}
	else
		channel->offset += size;
	CRIT_OUT(_p_lock)
	return size;
}

int32_t ipc_ready() {
	CRIT_IN(_p_lock)
	int32_t pid = _current_proc->pid;
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		channel_t* channel = &_channels[i];
		if(channel->ring < 0)
			continue;
		if(channel->proc_map[channel->ring].pid == pid) // current process hold the ring 
			break;
	}

	if(i >= CHANNEL_MAX) { // not channel ring for current proc
		proc_block(0xffffffff);
		CRIT_OUT(_p_lock)
		return -1;
	}
	CRIT_OUT(_p_lock)
	return i;
}

/*close all channels of specific process*/
void ipc_close_all(int32_t pid) {
	CRIT_IN(_p_lock)
	int32_t i;
	for(i=0; i<CHANNEL_MAX; i++) {
		channel_t* channel = &_channels[i];
		if(channel->ring < 0)
			continue;
		if(channel->proc_map[0].pid == pid || channel->proc_map[1].pid == pid) 
			ipc_close_raw(pid, channel);
	}
	CRIT_OUT(_p_lock)
}
