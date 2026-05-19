#define TAG "aplay"

#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"
#include "pcm.h"

#define UNUSED(v)	((void)v)
#define ARRAY_SIZE(A)	(sizeof(A)/sizeof(*A))

static unsigned char sineTone1KBuf[] = {
	0x00, 0x00, 0x5e, 0x0d, 0x80, 0x1a, 0x30, 0x27,
	0x33, 0x33, 0x57, 0x3e, 0x69, 0x48, 0x3b, 0x51,
	0xb1, 0x58, 0x98, 0x5e, 0xec, 0x62, 0x85, 0x65,
	0x66, 0x66, 0x88, 0x65, 0xe7, 0x62, 0x9c, 0x5e,
	0xae, 0x58, 0x3e, 0x51, 0x68, 0x48, 0x56, 0x3e,
	0x34, 0x33, 0x2e, 0x27, 0x84, 0x1a, 0x5a, 0x0d,
	0x03, 0x00, 0xa1, 0xf2, 0x80, 0xe5, 0xcf, 0xd8,
	0xce, 0xcc, 0xa8, 0xc1, 0x99, 0xb7, 0xc3, 0xae,
	0x50, 0xa7, 0x66, 0xa1, 0x16, 0x9d, 0x7b, 0x9a,
	0x98, 0x99, 0x7e, 0x9a, 0x11, 0x9d, 0x69, 0xa1,
	0x50, 0xa7, 0xc4, 0xae, 0x96, 0xb7, 0xac, 0xc1,
	0xca, 0xcc, 0xd3, 0xd8, 0x7e, 0xe5, 0xa1, 0xf2
};


int play_with_file(const char *fileName)
{
	int ret;
	long data_offset;
	long file_size;
	long data_size;
	size_t read_size = 0;
	size_t buffer_size = 0;
	char *buf = NULL;
	int rate, bit_depth, channels;

	FILE *file = fopen(fileName, "r");
	if (!file) {
		LOGE("fopen() error:%d\n", errno);
		return 0;
	}

	struct chunk_fmt wavFormat;
	ret = parse_wav_header(file, &wavFormat);
	if (ret < 0) {
		LOGE("%s() can't parse this WAV format!\n", __func__);
		fclose(file);
		return 0;
	}

	rate = wavFormat.sample_rate;
	bit_depth = wavFormat.bits_per_sample;
	channels = wavFormat.num_channels;
	data_offset = ftell(file);
	if (data_offset < 0) {
		LOGE("ftell() error:%d\n", errno);
		fclose(file);
		return -1;
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		LOGE("fseek() end error:%d\n", errno);
		fclose(file);
		return -1;
	}
	file_size = ftell(file);
	if (file_size < data_offset) {
		LOGE("invalid wav data offset:%ld size:%ld\n", data_offset, file_size);
		fclose(file);
		return -1;
	}
	data_size = file_size - data_offset;
	if (fseek(file, data_offset, SEEK_SET) != 0) {
		LOGE("fseek() data error:%d\n", errno);
		fclose(file);
		return -1;
	}

	struct pcm_config config = {
		.bit_depth = bit_depth,
		.rate = rate,
		.channels = channels,
		.period_size = 2048,
		.period_count = 4,
		.start_threshold = 2048 * channels,
		.stop_threshold = 0,
	};

	struct pcm *pcm = pcm_open("/dev/sound0", &config);
	if (pcm == NULL) {
		LOGE("pcm_open() fail, return!");
		return -1;
	}

	dump_pcm_config(&config);

	buffer_size = (size_t)data_size;
	buf = (char*)malloc(buffer_size);
	if (buf == NULL) {
		fclose(file);
		pcm_close(pcm);
		LOGE("%s() line:%d No memory, return!\n", __func__ ,__LINE__);
		return -1;
	}

	read_size = fread(buf, 1, buffer_size, file);
	if (read_size != buffer_size) {
		LOGW("%s() fread() short read size:%u expect:%u\n",
				__func__, (unsigned int)read_size, (unsigned int)buffer_size);
	}
	ret = 0;
	if (read_size > 0) {
		LOGV("pcm_write() size:%u", (unsigned int)read_size);
		ret = pcm_write(pcm, buf, (unsigned int)read_size);
		if (ret != 0) {
			LOGD("pcm_write() ERROR, ret:%d\n", ret);
		}
	}

	free(buf);
	fclose(file);
	pcm_close(pcm);
	return ret;
}

struct pq_buf_mgr {
	struct pcm *wpcm;
	int done;
	int pcm_exit;
	char *p_addr;
	int p_size;
	char *q_addr;
	int q_size;
};

void *push_data_to_pcm(void *data);

void start_playback(void *data)
{
	pthread_t tid;
	pthread_create(&tid, NULL, push_data_to_pcm, data);
}

void *push_data_to_pcm(void *data)
{
	struct pq_buf_mgr *mgr = (struct pq_buf_mgr*)data;
	struct pcm *wpcm = mgr->wpcm;
	int ret = 0;

	proc_usleep(200 * 1000);
	if ((mgr->p_size == 0) && (mgr->q_size == 0)) {
		LOGD("%s() No available data, should not happen!", __func__);
	}

	for(;;) {
		if (mgr->done) {
			break;
		}

		int p_size = mgr->p_size;
		int q_size = mgr->q_size;

		if ((p_size == 0) && (q_size == 0)) {
			//LOGD("%s() No available data, sleep and try again", __func__);
			proc_usleep(1000);
			continue;
		}

		if (p_size != 0) {
			//LOGD("%s() p_size %d", __func__, mgr->p_size);
			ret = pcm_write(wpcm, mgr->p_addr, mgr->p_size);
			mgr->p_size = 0;
			if (ret != 0) {
				LOGD("pcm_write() ERROR, ret:%d\n", ret);
				break;
			}
		}

		if (q_size != 0) {
			//LOGD("%s() q_size %d", __func__, mgr->q_size);
			ret = pcm_write(wpcm, mgr->q_addr, mgr->q_size);
			mgr->q_size = 0;
			if (ret != 0) {
				LOGD("pcm_write() ERROR, ret:%d\n", ret);
				break;
			}
		}
	}

	pcm_close(wpcm);
	mgr->pcm_exit = 1;
	return NULL;
}

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(sineTone1KBuf);
	char wavFileName[128] = {0};

	if(argc < 2) {
		LOGE("Usage: %s [-f wav_file]\n", argv[0]);
		return -1;
	}
	strncpy(wavFileName, argv[1], 127);

	/*while(i < argc) {
		if (strcmp(argv[i], "-f") == 0) {
			if (++i >= argc) {
				break;
			}
			if (argv[i] == NULL) {
				strncpy(wavFileName, "/data/test/test.wav", 127);
			}else {
				strncpy(wavFileName, argv[i], 127);
			}
			playWav = 1;
		} else if (strcmp(argv[i], "-r") == 0) {
			if (++i >= argc) {
				break;
			}
			rate = atoi(argv[i]);
		} else if (strcmp(argv[i], "-b") == 0) {
			if (++i >= argc) {
				break;
			}
			bit_depth = atoi(argv[i]);
		} else if (strcmp(argv[i], "-c") == 0) {
			if (++i >= argc) {
				break;
			}
			channels = atoi(*argv);
		} else {
			++i;
		}
	}
	*/

	play_with_file(wavFileName);

	LOGD("main() end pcm_close()\n");
	return 0;
}
