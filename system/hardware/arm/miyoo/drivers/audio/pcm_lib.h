
#ifndef PCM_LIB_H_
#define PCM_LIB_H_

#include <sys/types.h>
#include <ewoksys/vdevice.h>
#include <pthread.h>

#include "list.h"

#define CTRL_PCM_DEV_HW			(0xF0)
#define CTRL_PCM_DEV_HW_FREE		(0xF1)
#define CTRL_PCM_DEV_PRPARE		(0xF2)
#define CTRL_PCM_BUF_AVAIL		(0xF3)

enum pcm_type_t {
	PCM_TYPE_PLAYBACK,
	PCM_TYPE_CAPTURE,
	PCM_TYPE_LOOPBACK,
};

enum snd_device_type_t {
	SND_DEV_TYPE_PCM,
	SND_DEV_TYPE_CTRL,
};

enum dai_type_t {
	DAI_TYPE_MEMIF,
	DAI_TYPE_CPU_DAI,
	DAI_TYPE_CODEC_DAI,
	DAI_TYPE_CODEC,
};

enum snd_pcm_state_t {
	PCM_STATE_UNKOWN = 0,
	PCM_STATE_OPEN = 1 << 0,
	PCM_STATE_SETUP = 1 << 1,
	PCM_STATE_PREPARE = 1 << 2,
	PCM_STATE_RUNNING = 1 << 3,
	PCM_STATE_XRUN = 1 << 4,
	PCM_STATE_STOPED = 1 << 5,
	PCM_STATE_DROPD = 1 << 6,
};

enum pcm_triger_cmd_t {
	PCM_TRIGER_START,
	PCM_TRIGER_STOP,
};

struct pcm_config {
	int bit_depth;
	int rate;
	int channels;
	int period_size;
	int period_count;
	int start_threshold;
	int stop_threshold;
};

struct file_operation {
	int (*dev_cntl)(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p);
	int (*open)(int fd, int from_pid, fsinfo_t* info, int oflag, void* p);
	int (*create)(fsinfo_t* info_to, fsinfo_t* info, void* p);
	int (*close)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*read)(int fd, int from_pid, fsinfo_t* info, void* buf, int size, int offset, void* p);
	int (*write)(int fd, int from_pid, fsinfo_t* info, const void* buf, int size, int offset, void* p);
	int (*read_block)(int from_pid, void* buf, int size, int index, void* p);
	int (*write_block)(int from_pid, const void* buf, int size, int index, void* p);
	int (*dma)(int fd, int from_pid, fsinfo_t* info, int* size, void* p);
	int (*flush)(int fd, int from_pid, fsinfo_t* info, void* p);
	int (*fcntl)(int fd, int from_pid, fsinfo_t* info, int cmd, proto_t* in, proto_t* out, void* p);
	int (*mount)(fsinfo_t* mnt_point, void* p);
	int (*umount)(fsinfo_t* mnt_point, void* p);
	int (*unlink)(fsinfo_t* info, const char *fname, void* p);
	int (*clear_buffer)(fsinfo_t* info, void* p);
	void (*interrupt)(proto_t* in, void* p);
	void (*handled)(void* p);
	int (*loop_step)(void* p);
};

struct snd_card {
	char name[32];
	struct listnode pcm_list;
	int num_pcm;
	struct listnode dev_list;
	int dev_num;
};

struct snd_device {
	void *owner;
	void *param;
	int type;
	struct listnode list;
	int (*dev_new)(struct snd_device *dev);
	int (*dev_free)(struct snd_device *dev);
};

struct snd_pcm_substream;

struct snd_pcm {
	struct snd_card *card;
	struct snd_device *dev;
	char name[32];
	int type;
	int id;
	struct listnode list;
	//DAI list
	struct listnode dai_list;
	int num_dai;
	//substream list
	struct snd_pcm_substream *substream;
	struct snd_soc_dai *mem_dai;
	void *private_data;
	/* file operations: fops -> soc_ops -> dai_ops - substream ops*/
	void *vdev;
	void *fops;
};

struct shm_status {
	volatile int state;
	volatile int32_t hw_ptr;
	volatile int32_t appl_ptr;
};

struct snd_pcm_runtime {
	int32_t hw_ptr_base;
	int32_t hw_ptr_interrupt;
	/* HW config */
	uint32_t bit_depth;
	uint32_t rate;
	uint32_t channels;
	int frame_size; /* frame size in bytes */
	int32_t period_size; /* period_size*/
	int32_t periods; /*period_count */
	int32_t frame_bits;
	void *private_data;

	/* SW config */
	int32_t start_threshold;
	int32_t stop_threshold;
	int32_t boundary;
	int buffer_size;
	struct shm_status status;

	/* DMA */
	char *dma_area; /* Access by CPU */
	unsigned int dma_addr; /*physic bus adddress */
	int32_t dma_bytes;
	/* runing info */
	int ack_count;
	int irq_count;
	uint64_t time_pre;
	uint64_t time_after;
	uint64_t irq_usec;
};

struct snd_pcm_ops {
	int (*open)(struct snd_pcm_substream *substream);
	int (*close)(struct snd_pcm_substream *substream);
	int (*hw_params)(struct snd_pcm_substream *substream);
	int (*hw_free)(struct snd_pcm_substream *substream);
	int (*pointer)(struct snd_pcm_substream *substream);
	int (*prepare)(struct snd_pcm_substream *substream);
	int (*trigger)(struct snd_pcm_substream *substream, int cmd);
	int (*ack)(struct snd_pcm_substream *substream);
};

/* If your driver define DAI then use pre-defined snd_pcm_ops */
extern struct snd_pcm_ops soc_dai_pcm_ops;

struct snd_pcm_substream {
	char name[32];
	struct snd_pcm *pcm;
	void *private_data;
	int open_count;
	const struct snd_pcm_ops *ops;
	struct snd_pcm_runtime *runtime;

	pthread_mutex_t lock;
};

struct snd_soc_dai;

/* Hardware capabiltiy */
struct snd_pcm_hardware {
	uint32_t rates;
	uint32_t rates_min;
	uint32_t rate_max;
	uint32_t channels_min;
	uint32_t channels_max;
	int buffer_bytes_min;
	int buffer_bytes_max;
	int periods_min;
	int periods_max;
};

struct snd_soc_dai_ops {
	int (*open)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*close)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*hw_params)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*hw_free)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*prepare)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*trigger)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream, int cmd);
	int (*pointer)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
	int (*ack)(struct snd_soc_dai *dai, struct snd_pcm_substream *substream);
};

struct snd_soc_dai {
	char *name;
	int type;
	void *private_data; //substream
	struct snd_pcm_hardware *hardware;
	struct snd_soc_dai_ops *ops;
	struct listnode list;
};

int snd_pcm_lock(struct snd_pcm_substream *substream);
int snd_pcm_unlock(struct snd_pcm_substream *substream);
int snd_card_new(struct snd_card **card, const char *name);
int snd_card_free(struct snd_card *card);
int snd_card_register(struct snd_card *card);
int snd_card_unregister(struct snd_card *card);
int snd_card_info_print(struct snd_card *card);

int update_hw_ptr(struct snd_pcm_substream *substream, int is_interrupt);
int snd_pcm_new(struct snd_card *card, int type, int id, struct snd_pcm **rpcm);
int snd_pcm_free(struct snd_pcm *pcm);
int snd_set_pcm_ops(struct snd_pcm *pcm, struct snd_pcm_ops *ops);
int snd_add_dai(struct snd_pcm *pcm, struct snd_soc_dai *dai, int dai_type);

void snd_dump_substream(struct snd_pcm_substream *substream, int is_interrupt);

#define PCM_TYPE_TO_TAG(type) ((type == PCM_TYPE_PLAYBACK ? 'p' : \
				(type == PCM_TYPE_CAPTURE ? 'c' : 'l')))

static inline char *pcm_state_str(int state) {
	switch (state) {
	case PCM_STATE_OPEN:
		return "OPEN";
	case PCM_STATE_SETUP:
		return "SETUP";
	case PCM_STATE_PREPARE:
		return "PREPARED";
	case PCM_STATE_RUNNING:
		return "RUNNING";
	case PCM_STATE_XRUN:
		return "XRUN";
	case PCM_STATE_STOPED:
		return "STOPED";
	case PCM_STATE_DROPD:
		return "DROPED";
	default:
		return "Unkown";
	}
}

static inline void set_pcm_state(struct snd_pcm_substream *substream, int state)
{
	substream->runtime->status.state = state;
}

static inline int get_pcm_state(struct snd_pcm_substream *substream) {
	return substream->runtime->status.state;
}

static inline int32_t play_avail(struct snd_pcm_runtime *runtime) {
	int32_t avail = runtime->status.hw_ptr + runtime->buffer_size - runtime->status.appl_ptr;
	if (avail < 0) {
		avail += runtime->boundary;
	} else if (avail >= runtime->boundary) {
		avail -= runtime->boundary;
	}
	return avail;
}

static inline int32_t frames_ready(struct snd_pcm_runtime *runtime) {
	int frames = runtime->buffer_size - play_avail(runtime);
	if (frames >= 0) {
		return frames;
	} else {
		return 0;
	}
}

static inline int frame_to_bytes(struct snd_pcm_runtime *runtime, int frames)
{
	return (frames * runtime->channels * (runtime->bit_depth / 8));
}

static inline int bytes_to_frames(struct snd_pcm_runtime *runtime, int bytes) {
	return (bytes / (runtime->channels * (runtime->bit_depth / 8)));
}

#endif
