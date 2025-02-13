// minimp3 example player application for Linux/OSS
// this file is public domain -- do with it whatever you want!
#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"

int main(int argc, char **argv) {
	mp3dec_t mp3;
	mp3dec_frame_info_t info;
	void *file_data;
	unsigned char *stream_pos;
	signed short sample_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];
	int bytes_left;

	if(argc < 2) {
		fprintf(stderr, "Usage: %s [mp3_fname]\n", argv[0]);
		return 1;
	}

	int fd = open("/dev/sound", O_WRONLY);
	if(fd < 0) {
		fprintf(stderr, "/dev/sound device not ready!\n");
		return 1;
	}

	file_data = vfs_readfile(argv[1], &bytes_left);
	if(file_data == NULL) {
		fprintf(stderr, "Error: read MP3 audio file failed!\n");
		return 1;
	}
	stream_pos = (unsigned char *) file_data;

	mp3dec_init(&mp3);
	int simples = mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);

	if (simples == 0) {
		fprintf(stderr, "Error: not a valid MP3 audio file!\n");
		free(file_data);
		return 1;
	}

	fprintf(stderr, "sound write: %d ... \n", bytes_left);
	while ((bytes_left >= 0) && (simples > 0)) {
		stream_pos += info.frame_bytes;
		bytes_left -= info.frame_bytes;
		int sz = write(fd, (const void *) sample_buf, simples * 4);
		//fprintf(stderr, "sound write: %d on %d, left: %d\n", sz, size, bytes_left);
		simples = mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);
	}
	fprintf(stderr, "done.\n");
	free(file_data);
	close(fd);
	return 0;
}
