#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/virt/virtio.h>
#include <arch/virt/virtio_snd.h>
#include <ewoksys/klog.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>

#define UNUSED(v) ((void)(v))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CTRL_PCM_DEV_HW 0xF0
#define CTRL_PCM_DEV_HW_FREE 0xF1
#define CTRL_PCM_DEV_PRPARE 0xF2
#define CTRL_PCM_BUF_AVAIL 0xF3

#define SND_WRITE_TIMEOUT_MS 2000

struct pcm_config
{
	int bit_depth;
	int rate;
	int channels;
	int period_size;
	int period_count;
	int start_threshold;
	int stop_threshold;
};

typedef struct
{
	virtio_dev_t dev;
	struct virtio_snd_config config;
	struct virtio_snd_pcm_info stream_info;
	struct pcm_config pcm_cfg;
	uint32_t period_bytes;
	uint32_t buffer_bytes;
	int stream_id;
	int open_count;
	bool configured;
	bool prepared;
	bool started;
	int occupied_pid;
} snd_dev_t;

static snd_dev_t _snd = {.stream_id = -1};

static int snd_rate_to_virtio(int rate)
{
	switch (rate)
	{
	case 8000:
		return VIRTIO_SND_PCM_RATE_8000;
	case 16000:
		return VIRTIO_SND_PCM_RATE_16000;
	case 32000:
		return VIRTIO_SND_PCM_RATE_32000;
	case 44100:
		return VIRTIO_SND_PCM_RATE_44100;
	case 48000:
		return VIRTIO_SND_PCM_RATE_48000;
	case 96000:
		return VIRTIO_SND_PCM_RATE_96000;
	default:
		return -1;
	}
}

static int snd_format_to_virtio(int bit_depth)
{
	switch (bit_depth)
	{
	case 16:
		return VIRTIO_SND_PCM_FMT_S16;
	case 24:
		return VIRTIO_SND_PCM_FMT_S24_3;
	case 32:
		return VIRTIO_SND_PCM_FMT_S32;
	default:
		return -1;
	}
}

static int snd_select_output_stream(const struct pcm_config *cfg, int fmt_id, int rate_id,
									struct virtio_snd_pcm_info *chosen)
{
	uint64_t fmt_mask = 1ULL << fmt_id;
	uint64_t rate_mask = 1ULL << rate_id;

	for (uint32_t i = 0; i < _snd.config.streams; i++)
	{
		struct virtio_snd_pcm_info info;
		memset(&info, 0, sizeof(info));
		if (virtio_snd_query_pcm_info(_snd.dev, i, &info) != 0)
		{
			continue;
		}
		if (info.direction != VIRTIO_SND_D_OUTPUT)
		{
			continue;
		}
		if (cfg->channels < info.channels_min || cfg->channels > info.channels_max)
		{
			continue;
		}
		if ((info.formats & fmt_mask) == 0 || (info.rates & rate_mask) == 0)
		{
			continue;
		}

		memcpy(chosen, &info, sizeof(*chosen));
		return (int)i;
	}

	return -1;
}

static int snd_stop_and_release(bool reset_config)
{
	if (_snd.configured)
	{
		if (_snd.started)
		{
			(void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_STOP, (uint32_t)_snd.stream_id);
		}
		(void)virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_RELEASE, (uint32_t)_snd.stream_id);
	}

	virtio_snd_tx_reset(_snd.dev);
	virtio_snd_clear_error(_snd.dev);
	_snd.prepared = false;
	_snd.started = false;
	if (reset_config)
	{
		_snd.configured = false;
		_snd.stream_id = -1;
		memset(&_snd.pcm_cfg, 0, sizeof(_snd.pcm_cfg));
		memset(&_snd.stream_info, 0, sizeof(_snd.stream_info));
		_snd.period_bytes = 0;
		_snd.buffer_bytes = 0;
	}
	return 0;
}

static int snd_hw_params(const struct pcm_config *cfg)
{
	int fmt_id = snd_format_to_virtio(cfg->bit_depth);
	int rate_id = snd_rate_to_virtio(cfg->rate);
	struct virtio_snd_pcm_info info;

	if (fmt_id < 0 || rate_id < 0 || cfg->channels <= 0 ||
		cfg->period_size <= 0 || cfg->period_count <= 0)
	{
		return -1;
	}

	uint32_t frame_size = (uint32_t)(cfg->channels * cfg->bit_depth / 8);
	uint32_t period_bytes = (uint32_t)cfg->period_size * frame_size;
	uint32_t buffer_bytes = period_bytes * (uint32_t)cfg->period_count;
	if (frame_size == 0 || period_bytes == 0 || buffer_bytes == 0)
	{
		return -1;
	}

	int stream_id = snd_select_output_stream(cfg, fmt_id, rate_id, &info);
	if (stream_id < 0)
	{
		klog("virtio-snd: no output stream for %dch/%dbit/%dHz\n",
			 cfg->channels, cfg->bit_depth, cfg->rate);
		return -1;
	}

	snd_stop_and_release(true);

	if (virtio_snd_tx_init(_snd.dev,
						   MIN((uint32_t)cfg->period_count, (uint32_t)VIRTIO_SND_TX_SLOT_MAX),
						   period_bytes) != 0)
	{
		return -1;
	}

	if (virtio_snd_pcm_set_params(_snd.dev, (uint32_t)stream_id, buffer_bytes, period_bytes, 0,
								  (uint8_t)cfg->channels, (uint8_t)fmt_id, (uint8_t)rate_id) != 0)
	{
		virtio_snd_tx_reset(_snd.dev);
		return -1;
	}

	memcpy(&_snd.stream_info, &info, sizeof(info));
	memcpy(&_snd.pcm_cfg, cfg, sizeof(*cfg));
	_snd.period_bytes = period_bytes;
	_snd.buffer_bytes = buffer_bytes;
	_snd.stream_id = stream_id;
	_snd.configured = true;
	_snd.prepared = false;
	_snd.started = false;
	virtio_snd_clear_error(_snd.dev);
	return 0;
}

static int snd_prepare_stream(void)
{
	if (!_snd.configured)
	{
		return -1;
	}
	if (_snd.prepared)
	{
		return 0;
	}

	virtio_snd_tx_reset(_snd.dev);
	virtio_snd_clear_error(_snd.dev);
	if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_PREPARE, (uint32_t)_snd.stream_id) != 0)
	{
		return -1;
	}

	_snd.prepared = true;
	return 0;
}

static int snd_start_stream(void)
{
	if (!_snd.prepared)
	{
		return -1;
	}
	if (_snd.started)
	{
		return 0;
	}

	if (virtio_snd_pcm_ctl(_snd.dev, VIRTIO_SND_R_PCM_START, (uint32_t)_snd.stream_id) != 0)
	{
		return -1;
	}
	_snd.started = true;
	return 0;
}

static int snd_open(int fd, int from_pid, fsinfo_t *info, int oflag, void *p)
{
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(info);
	UNUSED(oflag);
	UNUSED(p);

	/*if (_snd.open_count > 0)
	{
		return -1;
	}
	*/
	snd_stop_and_release(true);
	_snd.open_count = 1;
	_snd.occupied_pid = from_pid;
	return 0;
}

static int snd_close(int fd, int from_pid, uint32_t node, fsinfo_t *info, void *p)
{
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(node);
	UNUSED(info);
	UNUSED(p);

	if (_snd.occupied_pid != from_pid) {
		return -1;
	}

	_snd.open_count = 0;
	snd_stop_and_release(true);
	return 0;
}

static int snd_write(int fd, int from_pid, fsinfo_t *node,
					 const void *buf, int size, int offset, void *p)
{
	UNUSED(fd);
	UNUSED(from_pid);
	UNUSED(node);
	UNUSED(p);

	if (!_snd.configured || size <= 0 || _snd.occupied_pid != from_pid)
	{
		return -1;
	}

	if (snd_prepare_stream() != 0 || snd_start_stream() != 0)
	{
		return -1;
	}

	int ret = virtio_snd_tx_write(_snd.dev, (uint32_t)_snd.stream_id,
								  (const uint8_t *)buf + offset, (uint32_t)size,
								  SND_WRITE_TIMEOUT_MS);
	if (ret < 0)
	{
		_snd.prepared = false;
		_snd.started = false;
	}
	return ret;
}

static int snd_dcntl(int from_pid, int cmd, proto_t *in, proto_t *ret, void *p)
{
	UNUSED(from_pid);
	UNUSED(p);

	int result = 0;
	struct pcm_config cfg;

	if (_snd.occupied_pid != from_pid) {
		return -1;
	}

	switch (cmd)
	{
	case CTRL_PCM_DEV_HW:
		memset(&cfg, 0, sizeof(cfg));
		proto_read_to(in, &cfg, sizeof(cfg));
		result = snd_hw_params(&cfg);
		break;
	case CTRL_PCM_DEV_HW_FREE:
		result = snd_stop_and_release(true);
		break;
	case CTRL_PCM_DEV_PRPARE:
		result = snd_prepare_stream();
		break;
	case CTRL_PCM_BUF_AVAIL:
		result = virtio_snd_poll(_snd.dev);
		if (result == 0 && _snd.configured)
		{
			result = virtio_snd_tx_avail_bytes(_snd.dev);
		}
		else if (result == 0)
		{
			result = -1;
		}
		break;
	default:
		result = -1;
		break;
	}

	PF->addi(ret, result);
	return 0;
}

static int snd_init_device(void)
{
	_mmio_base = mmio_map();
	_snd.dev = virtio_snd_get();
	if (_snd.dev == NULL)
	{
		klog("virtio-snd: no sound device found\n");
		return -1;
	}
	if (virtio_snd_init(_snd.dev) != 0 ||
		virtio_snd_read_config(_snd.dev, &_snd.config) != 0)
	{
		return -1;
	}
	if (_snd.config.streams == 0)
	{
		klog("virtio-snd: device reports zero pcm streams\n");
		return -1;
	}

	//klog("virtio-snd: streams=%u jacks=%u chmaps=%u\n",
		// _snd.config.streams, _snd.config.jacks, _snd.config.chmaps);
	return 0;
}

int main(int argc, char **argv)
{
	const char *mnt_point = argc > 1 ? argv[1] : "/dev/sound0";

	if (snd_init_device() != 0)
	{
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "virtio-snd");
	dev.open = snd_open;
	dev.close = snd_close;
	dev.write = snd_write;
	dev.dev_cntl = snd_dcntl;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
