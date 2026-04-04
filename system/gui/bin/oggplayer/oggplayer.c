// oggplayer - Ogg Vorbis player application
// Uses libvorbis and libogg for decoding

#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

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
	int period_bytes = pcm->config.period_size * pcm->config.channels * (pcm->config.bit_depth / 8);
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

	period_bytes = pcm->config.period_size * pcm->config.channels * (pcm->config.bit_depth / 8);
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

// OGG Vorbis file callbacks for vfs_readfile
typedef struct {
	uint8_t *data;
	int size;
	int pos;
} OggVorbis_DataSource;

static size_t ogg_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
	size_t bytes_to_read = size * nmemb;
	size_t bytes_available = ds->size - ds->pos;

	if (bytes_to_read > bytes_available) {
		bytes_to_read = bytes_available;
	}

	if (bytes_to_read > 0) {
		memcpy(ptr, ds->data + ds->pos, bytes_to_read);
		ds->pos += bytes_to_read;
	}

	return bytes_to_read;
}

static int ogg_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
	int new_pos = 0;

	switch (whence) {
		case SEEK_SET:
			new_pos = (int)offset;
			break;
		case SEEK_CUR:
			new_pos = ds->pos + (int)offset;
			break;
		case SEEK_END:
			new_pos = ds->size + (int)offset;
			break;
		default:
			return -1;
	}

	if (new_pos < 0 || new_pos > ds->size) {
		return -1;
	}

	ds->pos = new_pos;
	return 0;
}

static int ogg_close_func(void *datasource)
{
	OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
	if (ds->data != NULL) {
		free(ds->data);
		ds->data = NULL;
	}
	return 0;
}

static long ogg_tell_func(void *datasource)
{
	OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
	return ds->pos;
}

int ogg_play_file(const char *path, const char *snd_dev) {
	OggVorbis_File vf;
	vorbis_info *vi;
	vorbis_comment *vc;
	OggVorbis_DataSource ds;
	ov_callbacks callbacks;

	int file_size;
	uint8_t *file_data = (uint8_t *)vfs_readfile(path, &file_size);
	if (file_data == NULL) {
		printf("read %s failed\n", path);
		return 1;
	}

	ds.data = file_data;
	ds.size = file_size;
	ds.pos = 0;

	callbacks.read_func = ogg_read_func;
	callbacks.seek_func = ogg_seek_func;
	callbacks.close_func = ogg_close_func;
	callbacks.tell_func = ogg_tell_func;

	if (ov_open_callbacks(&ds, &vf, NULL, 0, callbacks) < 0) {
		printf("Not an Ogg Vorbis audio stream\n");
		free(file_data);
		return 1;
	}

	vi = ov_info(&vf, -1);
	vc = ov_comment(&vf, -1);

	int channels = vi->channels;
	if (channels != 1 && channels != 2) {
		channels = 2;
	}

	int rate = vi->rate;
	if (rate != 8000 && rate != 16000 && rate != 32000 &&
		rate != 44100 && rate != 48000 && rate != 96000) {
		rate = 44100;
	}

	// Force stereo output
	int output_channels = 2;

	struct pcm_config config = {
		.bit_depth = 16,
		.rate = rate,
		.channels = output_channels,
		.period_size = 2048,
		.period_count = 4,
		.start_threshold = 2048 * output_channels,
		.stop_threshold = 0,
	};

	struct pcm *pcm = pcm_open(snd_dev, &config);
	if (pcm == NULL) {
		printf("pcm_open failed: rate=%d, channels=%d\n", rate, output_channels);
		ov_clear(&vf);
		free(file_data);
		return 1;
	}

	// Buffer for decoded audio (max ~4096 samples * 2 bytes = 8192)
	char buffer[8192];
	// Stereo buffer needs 2x the mono samples
	char stereo_buffer[16384];

	printf("Playing: %s\n", path);
	printf("Channels: %d, Rate: %d Hz\n", channels, rate);

	int16_t *mono_buf;
	int16_t *stereo_buf;
	int samples;

	int bitstream;
	while (1) {
		long bytes_read = ov_read(&vf, buffer, sizeof(buffer), 0, 2, 1, &bitstream);

		if (bytes_read == 0) {
			break;
		}

		if (bytes_read < 0) {
			printf("Error decoding Ogg Vorbis stream\n");
			break;
		}

		// Convert mono to stereo if needed
		if (channels == 1) {
			mono_buf = (int16_t *)buffer;
			stereo_buf = (int16_t *)stereo_buffer;
			samples = bytes_read / 2;
			for (int i = samples - 1; i >= 0; i--) {
				stereo_buf[i * 2] = mono_buf[i];
				stereo_buf[i * 2 + 1] = mono_buf[i];
			}
			int ret = pcm_write(pcm, stereo_buffer, samples * 4);
			if (ret != 0) {
				printf("pcm_write failed, ret=%d\n", ret);
				break;
			}
		} else {
			int ret = pcm_write(pcm, buffer, bytes_read);
			if (ret != 0) {
				printf("pcm_write failed, ret=%d\n", ret);
				break;
			}
		}
	}

	pcm_close(pcm);
	ov_clear(&vf);
	free(file_data);

	return 0;
}

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [ogg_fname] [snd_dev]\n", argv[0]);
		return 1;
	}

	if(argc >= 3) {
		return ogg_play_file(argv[1], argv[2]);
	} else {
		return ogg_play_file(argv[1], "/dev/sound0");
	}
}