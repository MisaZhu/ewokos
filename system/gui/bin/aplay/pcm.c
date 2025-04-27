
#define TAG "asound-lib"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>

#include "pcm.h"
#include "utils.h"

#define	EPIPE					(32)

#define CTRL_PCM_DEV_HW				(0xF0)
#define CTRL_PCM_DEV_HW_FREE			(0xF1)
#define CTRL_PCM_DEV_PRPARE			(0xF2)
#define CTRL_PCM_BUF_AVAIL			(0xF3)

static int pcm_prepare(struct pcm *pcm);

static int pcm_param_set(struct pcm *pcm, struct pcm_config *config)
{
	proto_t in, out;
	PF->init(&in)->add(&in, config, sizeof(struct pcm_config));
	PF->init(&out);
	int ret = 0;
	LOGV("+pcm_param_set() in_size:%d", sizeof(struct pcm_config));
	ret = dev_cntl(pcm->name, CTRL_PCM_DEV_HW, &in, &out);
	LOGV("-pcm_param_set() in_size:%d", sizeof(struct pcm_config));
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
		return true;
	}
	return false;
}

static int support_channels(unsigned int channels) {
	if (channels != 2) {
		return false;
	}
	return true;
}

static int support_bit_depth(unsigned int bit_depth) {
	switch (bit_depth)
	{
	case 16:
	case 24:
	case 32:
		return true;
	default:
		return false;
	}
}

static int is_valid_config(struct pcm_config *config)
{
	if (!support_bit_depth(config->bit_depth) ||
		!support_channels(config->channels) ||
		!support_rate(config->rate)) {
		LOGE("%s() Unsupport config! bit:%d channels:%d rate:%d\n", __func__,
			config->bit_depth, config->channels, config->rate);
		return false;
	}

	if (config->period_size == 0 || config->period_count == 0) {
		LOGE("%s() Unsupport config! period_size:%d period_count:%d\n", __func__,
			config->period_size, config->period_count);
		return false;
	}

	if (config->start_threshold == 0) {
		config->start_threshold = config->period_size;
	}

	if (config->stop_threshold == 0) {
		config->stop_threshold = config->period_size * config->period_count;
	}
	return true;
}

struct pcm* pcm_open(const char *name, struct pcm_config *config)
{
	struct pcm* pcm;

	if (!is_valid_config(config)) {
		LOGE("%s() Error! Can't support config!", __func__);
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
		LOGE("can not open pcm device:%s\n", name);
		return NULL;
	}

	int temp = pcm_param_set(pcm, &pcm->config);
	if (temp != 0) {
		LOGE("hw_parm() fail! return!\n");
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
				LOGW("%s() 1st write(fd) Fail!\n", __func__);
				return -1;
			}
			pcm->running = 1;
			return 0;
		}

		int ret = write(pcm->fd, data, count);
		return (ret == (int)count ? 0 : -1);
	}
}

/*
* return value:
* 	ret >= 0: mean PCM is running, available size = ret
*	ret < 0: mean PCM enter not running state, then can't write again
*/
int wait_avail(struct pcm *pcm, int *avail, int time_out_ms)
{
	enum {
		SLEEP_TIME_MS = 5,
	};
	*avail = 0;
	int ret = 0;
	int period_bytes = pcm->config.period_size * 4;
	int max_try_count = time_out_ms /SLEEP_TIME_MS;
	int try_count = 0;

	for(;;) {
		ret = pcm_buf_avail(pcm);
		if (ret < 0) {
			LOGW("%s() PCM Not Running! break! err:%d\n",__func__, ret);
			break;
		}

		if (ret >= period_bytes) {
			*avail = ret;
			break;
		}

		if(try_count++ >= max_try_count) {
			LOGW("%s() Timeout %dMs, count %d\n",__func__, time_out_ms, try_count);
			break;
		}

		if (pcm->hook != NULL) {
			pcm->hook(pcm->private);
		} else {
			proc_usleep(SLEEP_TIME_MS * 1000); /* Try again until timeout */
		}
	}

	return ret;
}

int pcm_write(struct pcm *pcm, const void* data, unsigned int count) {
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
		ret = wait_avail(pcm, &avail, 2000/*Ms*/);
		if (ret == -EPIPE) {
			copy_bytes = (bytes < period_bytes ? bytes : period_bytes);
			pcm->prepared = 0;
			pcm->running = 0;
			/*If hanppen xrun then go 1st write*/
			continue;
		}

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


int pcm_close(struct pcm *pcm)
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
		return 0; //already prepared
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
	} else {
		LOGE("pcm_prepare() fail! ret =%d\n", ret);
	}
	return ret;
}

void set_pcm_hook(struct pcm *pcm, pcm_hook_t func, void *data)
{
	pcm->hook = func;
	pcm->private = data;
}

void dump_pcm_config(struct pcm_config *config) {
    LOGD("dump_pcm_config() bits %d rate %d channels %d period_size %d period_count %d start_threshold %d stop_threshold%d\n",
	config->bit_depth,
	config->rate,
	config->channels,
	config->period_size,
	config->period_count,
	config->start_threshold,
	config->stop_threshold);
}