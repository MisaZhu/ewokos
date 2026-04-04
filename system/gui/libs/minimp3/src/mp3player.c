// minimp3 example player application for Linux/OSS
// this file is public domain -- do with it whatever you want!
#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct pcm {
	int fd;
	int prepared;
	int running;
	char name[32];
	int framesize;
	struct pcm_config {
		int bit_depth;
		int rate;
		int channels;
		int period_size;
		int period_count;
		int start_threshold;
		int stop_threshold;
	} config;
	int (*hook)(void *data);
	void *private;
};

typedef int (*pcm_hook_t)(void *data);

#define CTRL_PCM_DEV_HW			(0xF0)
#define CTRL_PCM_DEV_PRPARE		(0xF2)
#define CTRL_PCM_BUF_AVAIL		(0xF3)

static int pcm_prepare(struct pcm *pcm);

static int pcm_param_set(struct pcm *pcm, struct pcm_config *config)
{
	proto_t in, out;
	PF->init(&in)->add(&in, config, sizeof(struct pcm_config));
	PF->init(&out);
	int ret = 0;
	ret = dev_cntl(pcm->name, CTRL_PCM_DEV_HW, &in, &out);
	if(ret == 0) {
		ret = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

static int pcm_buf_avail(struct pcm *pcm)
{
	proto_t in, out;
	PF->init(&in);
	PF->init(&out);
	int ret = 0;
	ret = dev_cntl(pcm->name, CTRL_PCM_BUF_AVAIL, &in, &out);
	if(ret == 0) {
		ret = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

static int support_rate(unsigned int rate) {
	switch (rate) {
	case 8000:
	case 16000:
	case 32000:
	case 44100:
	case 48000:
	case 96000:
		return 1;
	}
	return 0;
}

static int support_channels(unsigned int channels) {
	if (channels != 2) {
		return 0;
	}
	return 1;
}

static int support_bit_depth(unsigned int bit_depth) {
	switch (bit_depth) {
	case 16:
	case 24:
	case 32:
		return 1;
	default:
		return 0;
	}
}

static int is_valid_config(struct pcm_config *config)
{
	if (!support_bit_depth(config->bit_depth) ||
		!support_channels(config->channels) ||
		!support_rate(config->rate)) {
		return 0;
	}
	if (config->period_size == 0 || config->period_count == 0) {
		return 0;
	}
	if (config->start_threshold == 0) {
		config->start_threshold = config->period_size;
	}
	if (config->stop_threshold == 0) {
		config->stop_threshold = config->period_size * config->period_count;
	}
	return 1;
}

static struct pcm* pcm_open(const char *name, struct pcm_config *config)
{
	struct pcm* pcm;

	if (!is_valid_config(config)) {
		return NULL;
	}

	pcm = calloc(1, sizeof(struct pcm));
	if (pcm == NULL) {
		return NULL;
	}

	strncpy(pcm->name, name, 31);
	memcpy(&pcm->config, config, sizeof(struct pcm_config));
	pcm->framesize = config->channels * config->bit_depth / 8;

	pcm->fd = open(name, O_RDWR);
	if (pcm->fd < 0) {
		free(pcm);
		return NULL;
	}

	int temp = pcm_param_set(pcm, &pcm->config);
	if (temp != 0) {
		close(pcm->fd);
		free(pcm);
		return NULL;
	}

	return pcm;
}

static int pcm_try_write(struct pcm *pcm, const void* data, unsigned int count)
{
	if (count == 0) {
		return 0;
	}

	for (;;) {
		if (pcm->running == 0) {
			int err = pcm_prepare(pcm);
			if (err != 0) {
				return err;
			}

			int written = write(pcm->fd, data, count);
			if (written != (int)count) {
				return -1;
			}
			pcm->running = 1;
			return 0;
		}

		int ret = write(pcm->fd, data, count);
		return (ret == (int)count ? 0 : -1);
	}
}

static int wait_avail(struct pcm *pcm, int *avail, int time_out_ms)
{
	enum {
		SLEEP_TIME_MS = 5,
	};
	*avail = 0;
	int ret = 0;
	int period_bytes = pcm->config.period_size * 4;
	int max_try_count = time_out_ms / SLEEP_TIME_MS;
	int try_count = 0;

	for(;;) {
		ret = pcm_buf_avail(pcm);
		if (ret < 0) {
			break;
		}

		if (ret >= period_bytes) {
			*avail = ret;
			break;
		}

		if(try_count++ >= max_try_count) {
			break;
		}

		if (pcm->hook != NULL) {
			pcm->hook(pcm->private);
		} else {
			proc_usleep(SLEEP_TIME_MS * 1000);
		}
	}

	return ret;
}

static int pcm_write(struct pcm *pcm, const void* data, unsigned int count) {
	int period_bytes = 0;
	int avail = 0;
	int bytes = (int)count;
	int written = 0;
	int offset = 0;
	int copy_bytes = 0;
	int ret = 0;

	period_bytes = pcm->config.period_size * 4;
	copy_bytes = bytes < period_bytes ? bytes : period_bytes;
	while (bytes > 0) {
		ret = wait_avail(pcm, &avail, 2000);
		if (ret < 0 || (avail == 0)) {
			break;
		}

		copy_bytes = bytes < avail ? bytes : avail;

		ret = pcm_try_write(pcm, data + offset, copy_bytes);
		if (ret == 0) {
			offset += copy_bytes;
			written += copy_bytes;
			bytes -= copy_bytes;
			copy_bytes = bytes < period_bytes ? bytes : period_bytes;
		}
	}

	return (written == (int)count ? 0 : -1);
}

static int pcm_close(struct pcm *pcm)
{
	if (pcm == NULL) {
		return 0;
	}
	close(pcm->fd);
	free(pcm);
	return 0;
}

static int pcm_prepare(struct pcm *pcm)
{
	if (pcm->prepared) {
		return 0;
	}

	proto_t in, out;
	PF->init(&in);
	PF->init(&out);
	int ret = dev_cntl(pcm->name, CTRL_PCM_DEV_PRPARE, &in, &out);
	if(ret == 0) {
		ret = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);

	if (ret == 0) {
		pcm->prepared = 1;
	}
	return ret;
}

int mp3_play_file(const char *path, const char *snd_dev) {
	mp3dec_t mp3;
	mp3dec_frame_info_t info;
	void *file_data;
	unsigned char *stream_pos;
	signed short sample_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
	int bytes_left;

	file_data = vfs_readfile(path, &bytes_left);
	if(file_data == NULL) {
		printf("read %s failed\n", path);
		return 1;
	}
	stream_pos = (unsigned char *) file_data;

	mp3dec_init(&mp3);
	int simples = mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);

	if (simples == 0) {
		printf("decode %s failed\n", path);
		free(file_data);
		return 1;
	}

	int channels = info.channels;
	if (channels != 1 && channels != 2) {
		channels = 2;
	}

	int rate = info.hz;
	if (rate != 8000 && rate != 16000 && rate != 32000 &&
		rate != 44100 && rate != 48000 && rate != 96000) {
		rate = 44100;
	}

	struct pcm_config config = {
		.bit_depth = 16,
		.rate = rate,
		.channels = channels,
		.period_size = 2048,
		.period_count = 4,
		.start_threshold = 2048 * channels,
		.stop_threshold = 0,
	};

	struct pcm *pcm = pcm_open(snd_dev, &config);
	if (pcm == NULL) {
		printf("pcm_open failed: rate=%d, channels=%d\n", rate, channels);
		free(file_data);
		return 1;
	}

	stream_pos += info.frame_bytes;
	bytes_left -= info.frame_bytes;

	while ((bytes_left >= 0) && (simples > 0)) {
		int ret = pcm_write(pcm, sample_buf, simples * 2 * info.channels);
		if (ret != 0) {
			printf("pcm_write failed, ret=%d\n", ret);
			break;
		}
		simples = mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);
		stream_pos += info.frame_bytes;
		bytes_left -= info.frame_bytes;
	}

	free(file_data);
	pcm_close(pcm);
	return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */