
#include <sys/types.h>

struct pcm_config {
	int bit_depth; /* bit of one sample */
	int rate;	/* sample rate */
	int channels;
	int period_size; /* the frame size in one period */
	int period_count; /* period count for whole buffer */
	int start_threshold;
	int stop_threshold;
};

struct pcm {
	int fd;
	int prepared;
	int running;
	char name[32];
	int framesize;
	struct pcm_config config;

	/* make the best of process pending resource */
	int (*hook)(void *data);
	void *private;
};

typedef int (*pcm_hook_t)(void *data);

struct pcm* pcm_open(const char *name, struct pcm_config *config);

void set_pcm_hook(struct pcm *pcm, pcm_hook_t func, void *data);

int pcm_write(struct pcm *pcm, const void* data, unsigned int count);

int pcm_close(struct pcm *pcm);

void dump_pcm_config(struct pcm_config *config);
