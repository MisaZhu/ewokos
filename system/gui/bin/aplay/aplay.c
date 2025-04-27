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
	int read_size = 0;
	int buffer_size = 0;
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

	struct pcm_config config = {
		.bit_depth = bit_depth,
		.rate = rate,
		.channels = channels,
		.period_size = 2048,
		.period_count = 2,
		.start_threshold = 2048 * 2,
		.stop_threshold = 0,
	};

	struct pcm *pcm = pcm_open("/dev/sound", &config);
	if (pcm == NULL) {
		LOGD("pcm_open() fail, return!");
		return -1;
	}

	dump_pcm_config(&config);

	buffer_size = config.period_size * 4;
	buf = (char*)malloc(buffer_size);
	if (buf == NULL) {
		fclose(file);
		pcm_close(pcm);
		LOGE("%s() line:%d No memory, return!\n", __func__ ,__LINE__);
		return -1;
	}

	for (;;) {
		LOGV("pcm_write() size:%d", buffer_size);
		int r_sz= fread(buf + read_size, 1, 1024, file);
		if (r_sz <= 0) {
			LOGW("%s() fread() EOF! read_size:%d\n", __func__, read_size);
			break;
		}

		read_size += r_sz;
		if (read_size < buffer_size) {
			continue;;
		}

		ret = pcm_write(pcm, buf, read_size);
		if (ret != 0) {
			LOGD("pcm_write() ERROR, ret:%d\n", ret);
			break;
		}

		read_size = 0;
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

int play_with_stdin(int bit_depth, int rate, int channels)
{
	int ret = 0;
	int read_size = 0;
	int buffer_size = 0;
	char *pbuf = NULL;
	char *qbuf = NULL;
	FILE *file = stdin;

	struct pcm_config config = {
		.bit_depth = bit_depth,
		.rate = rate,
		.channels = channels,
		.period_size = 2048,
		.period_count = 2,
		.start_threshold = 2048 * 2,
		.stop_threshold = 0,
	};

	struct pcm *pcm = pcm_open("/dev/sound", &config);
	if (!pcm) {
		LOGD("pcm_open() fail, return!");
		return -1;
	}

	dump_pcm_config(&config);

	buffer_size = config.period_size * 4;
	pbuf = (char*)malloc(buffer_size);
	if (!pbuf) {
		fclose(file);
		pcm_close(pcm);
		LOGE("%s() line:%d No memory, return!\n", __func__ ,__LINE__);
		return -1;
	}

	qbuf = (char*)malloc(buffer_size);
	if (!qbuf) {
		fclose(file);
		pcm_close(pcm);
		LOGE("%s() line:%d No memory, return!\n", __func__ ,__LINE__);
		return -1;
	}

	struct pq_buf_mgr mgr;
	mgr.wpcm = pcm;
	mgr.pcm_exit = 0;
	mgr.done = 0;
	mgr.p_addr = pbuf;
	mgr.p_size = 0;
	mgr.q_addr = qbuf;
	mgr.q_size = 0;

	start_playback(&mgr);

	int read_all = 0;
	for (;;) {
		int p_size = mgr.p_size;
		int q_size = mgr.q_size;

		if (p_size == 0) {
			//LOGD("+%s() req_p %d", __func__, buffer_size);
			int r_sz = fread(mgr.p_addr + read_all, 1, 8192, stdin);
			if (r_sz <= 0) {
				LOGD("%s() fread() EOF\n", __func__);
				mgr.done = 1;
			}

			read_all += r_sz;
			if (read_all < buffer_size) {
				continue;
			}

			mgr.p_size = read_all;
			read_all = 0;
			//LOGD("-%s() p_size %d", __func__, mgr.p_size);
		}

		if (q_size == 0) {
			//LOGD("+%s() req_q %d", __func__, buffer_size);
			int r_sz = fread(mgr.q_addr + read_all, 1, 8192, stdin);
			if (r_sz <= 0) {
				LOGD("%s() fread() EOF\n", __func__);
				mgr.done = 1;
			}

			read_all += r_sz;
			if (read_all < buffer_size) {
				continue;
			}

			mgr.q_size = read_all;
			read_all = 0;
			//LOGD("-%s() q_size %d", __func__, mgr.q_size);
		}
	}

	while(mgr.pcm_exit != 0) {
		proc_usleep(10 * 1000);
	}

	free(pbuf);
	free(qbuf);
	fclose(file);
	return ret;
}

int main(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(sineTone1KBuf);
	int ret;
	char wavFileName[32] = {0};
	int playWav = 0;
	int rate = 48000; /* Default sample rate */
	int channels = 2; /* Default channel number */
	int bit_depth = 16; /* Default bit depth */
	FILE *file = NULL;
	int i = 0;

	while(i < argc) {
		if (strcmp(argv[i], "-f") == 0) {
			if (++i >= argc) {
				break;
			}
			if (argv[i] == NULL) {
				strncpy(wavFileName, "/data/test/test.wav", 31);
			}else {
				strncpy(wavFileName, argv[i], 31);
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

	if (playWav) {
		play_with_file(wavFileName);
	} else {
		play_with_stdin(bit_depth, rate, channels);
	}

	LOGD("main() end pcm_close()\n");
	return 0;
}