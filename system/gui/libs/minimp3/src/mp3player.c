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
#define PCM_WAIT_SLEEP_MS       1
#define MP3_WRITE_CHUNK_FRAMES  4608
#define MP3_STREAM_BUFFER_SIZE  (32 * 1024)
#define MP3_STREAM_REFILL_LOW   4096

#ifndef EPIPE
#define EPIPE 32
#endif

static int pcm_prepare(struct pcm *pcm);

static int mp3_stream_refill(int fd, uint8_t *buf, int buf_size,
		uint8_t **stream_pos, int *bytes_left, int *eof)
{
	int carry = *bytes_left;
	int total = 0;

	if (carry < 0) {
		carry = 0;
	}

	if (carry > 0 && *stream_pos != buf) {
		memmove(buf, *stream_pos, (size_t)carry);
	}
	*stream_pos = buf;
	total = carry;

	while (total < buf_size) {
		int rd;

		rd = read(fd, buf + total, buf_size - total);
		if (rd < 0) {
			return -1;
		}
		if (rd == 0) {
			*eof = 1;
			break;
		}
		total += rd;
		if (total >= MP3_STREAM_REFILL_LOW) {
			break;
		}
	}

	*bytes_left = total;
	return 0;
}

static int mp3_stream_decode_frame(mp3dec_t *mp3, int fd,
		uint8_t *stream_buf, int stream_buf_size, uint8_t **stream_pos,
		int *bytes_left, int *eof, mp3d_sample_t *sample_buf,
		mp3dec_frame_info_t *info)
{
	for (;;) {
		int decoded_samples;

		if (*bytes_left < MP3_STREAM_REFILL_LOW && !*eof) {
			if (mp3_stream_refill(fd, stream_buf, stream_buf_size,
					stream_pos, bytes_left, eof) != 0) {
				return -1;
			}
		}

		if (*bytes_left <= 0) {
			return 0;
		}

		decoded_samples = mp3dec_decode_frame(mp3, *stream_pos, *bytes_left,
				sample_buf, info);
		if (info->frame_bytes > 0) {
			*stream_pos += info->frame_bytes;
			*bytes_left -= info->frame_bytes;
			if (decoded_samples > 0) {
				return decoded_samples;
			}
			continue;
		}

		if (*eof) {
			return 0;
		}

		/*
		 * Keep making forward progress even if the decoder cannot
		 * resync on the current window.
		 */
		if (*bytes_left >= stream_buf_size) {
			(*stream_pos)++;
			(*bytes_left)--;
		}

		if (mp3_stream_refill(fd, stream_buf, stream_buf_size,
				stream_pos, bytes_left, eof) != 0) {
			return -1;
		}
	}
}

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

	/*
	 * Returns bytes actually written (>= 0) or a negative errno. The
	 * server uses partial-write semantics: an XRUN mid-write returns
	 * the bytes consumed so far, so the caller must advance by the
	 * returned count instead of treating a short write as an error.
	 */
	if (pcm->running == 0) {
		int err = pcm_prepare(pcm);
		if (err != 0) {
			return err;
		}

		int written = write(pcm->fd, data, count);
		if (written > 0) {
			pcm->running = 1;
		}
		return written;
	}

	return write(pcm->fd, data, count);
}

static int wait_avail(struct pcm *pcm, int *avail, int time_out_ms)
{
	*avail = 0;
	int ret = 0;
	int period_bytes = pcm->config.period_size * pcm->framesize;
	int min_avail = period_bytes / 4;
	int max_try_count = time_out_ms / PCM_WAIT_SLEEP_MS;
	int try_count = 0;

	if (min_avail < pcm->framesize) {
		min_avail = pcm->framesize;
	}

	for(;;) {
		ret = pcm_buf_avail(pcm);
		if (ret < 0) {
			break;
		}

		if (ret >= min_avail) {
			*avail = ret;
			break;
		}

		if(try_count++ >= max_try_count) {
			break;
		}

		if (pcm->hook != NULL) {
			pcm->hook(pcm->private);
		} else {
			//proc_usleep(PCM_WAIT_SLEEP_MS * 1000);
			proc_yield();
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
	int xrun_retry = 0;

	period_bytes = pcm->config.period_size * pcm->framesize;
	copy_bytes = bytes < period_bytes ? bytes : period_bytes;
	while (bytes > 0) {
		ret = wait_avail(pcm, &avail, 2000);
		if (ret == -EPIPE) {
			/*
			 * XRUN: the server keeps returning -EPIPE until we
			 * re-prepare, so recover instead of dropping the rest
			 * of the stream. Bound retries to avoid spinning if
			 * prepare keeps failing.
			 */
			if (xrun_retry++ >= 5) {
				break;
			}
			pcm->prepared = 0;
			pcm->running = 0;
			if (pcm_prepare(pcm) != 0) {
				proc_usleep(10000);
			}
			continue;
		}

		if (ret < 0 || (avail == 0)) {
			break;
		}

		copy_bytes = bytes < avail ? bytes : avail;

		ret = pcm_try_write(pcm, data + offset, copy_bytes);
		if (ret == -EPIPE) {
			/* XRUN hit inside write(): recover the same way */
			if (xrun_retry++ >= 5) {
				break;
			}
			pcm->prepared = 0;
			pcm->running = 0;
			if (pcm_prepare(pcm) != 0) {
				proc_usleep(10000);
			}
			continue;
		}
		if (ret < 0) {
			break;
		}
		/*
		 * Only real write progress proves the XRUN recovery worked;
		 * wait_avail succeeds right after re-prepare even when the
		 * engine is wedged, so resetting there defeats the retry cap.
		 */
		if (ret > 0) {
			xrun_retry = 0;
		}
		offset += ret;
		written += ret;
		bytes -= ret;
		copy_bytes = bytes < period_bytes ? bytes : period_bytes;
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
	uint8_t *stream_buf = NULL;
	uint8_t *stream_pos = NULL;
	signed short sample_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
	int16_t *chunk = NULL;
	int fd = -1;
	int bytes_left;
	int eof = 0;
	int samples = 0;
	int chunk_frames = 0;
	int ret = 0;

	fd = open(path, O_RDONLY);
	if(fd < 0) {
		printf("open %s failed\n", path);
		return 1;
	}

	stream_buf = (uint8_t *)malloc(MP3_STREAM_BUFFER_SIZE);
	if (stream_buf == NULL) {
		printf("alloc stream buffer failed\n");
		close(fd);
		return 1;
	}
	stream_pos = stream_buf;
	bytes_left = 0;

	mp3dec_init(&mp3);
	samples = mp3_stream_decode_frame(&mp3, fd, stream_buf,
			MP3_STREAM_BUFFER_SIZE, &stream_pos, &bytes_left, &eof,
			sample_buf, &info);

	if (samples <= 0) {
		printf("decode %s failed\n", path);
		free(stream_buf);
		close(fd);
		return 1;
	}

	int src_channels = info.channels;
	if (src_channels != 1 && src_channels != 2) {
		src_channels = 2;
	}
	int output_channels = 2;

	int rate = info.hz;
	if (rate != 8000 && rate != 16000 && rate != 32000 &&
		rate != 44100 && rate != 48000 && rate != 96000) {
		rate = 44100;
	}

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
		free(stream_buf);
		close(fd);
		return 1;
	}

	/*
	 * Stream decode->write with a small fixed chunk instead of decoding
	 * the whole file up front. A 3-minute 44.1kHz stereo track expands
	 * to ~30MB of PCM, which exhausts memory on miyoo mid-song and made
	 * long tracks abort halfway; a fixed chunk keeps the footprint
	 * constant regardless of track length.
	 */
	chunk = (int16_t *)malloc((size_t)MP3_WRITE_CHUNK_FRAMES *
			(size_t)output_channels * sizeof(int16_t));
	if (chunk == NULL) {
		printf("alloc pcm buffer failed\n");
		free(stream_buf);
		close(fd);
		pcm_close(pcm);
		return 1;
	}

	while (samples > 0) {
		if (chunk_frames + samples > MP3_WRITE_CHUNK_FRAMES) {
			if (pcm_write(pcm, chunk, (unsigned int)(chunk_frames *
					output_channels * (int)sizeof(int16_t))) != 0) {
				printf("pcm_write failed\n");
				ret = 1;
				break;
			}
			chunk_frames = 0;
		}
		for (int i = 0; i < samples; i++) {
			int16_t left = sample_buf[i * src_channels];
			int16_t right = (src_channels > 1) ? sample_buf[i * src_channels + 1] : left;

			chunk[(chunk_frames + i) * output_channels] = left;
			chunk[(chunk_frames + i) * output_channels + 1] = right;
		}
		chunk_frames += samples;

		samples = mp3_stream_decode_frame(&mp3, fd, stream_buf,
				MP3_STREAM_BUFFER_SIZE, &stream_pos, &bytes_left, &eof,
				sample_buf, &info);
		if (samples < 0) {
			printf("decode %s failed during playback\n", path);
			ret = 1;
			break;
		}
		src_channels = info.channels;
		if (src_channels != 1 && src_channels != 2) {
			src_channels = output_channels;
		}
	}

	if (ret == 0 && chunk_frames > 0) {
		ret = pcm_write(pcm, chunk, (unsigned int)(chunk_frames *
				output_channels * (int)sizeof(int16_t)));
		if (ret != 0) {
			printf("pcm_write failed, ret=%d\n", ret);
		}
	}

	free(chunk);
	free(stream_buf);
	close(fd);
	pcm_close(pcm);
	return ret;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
