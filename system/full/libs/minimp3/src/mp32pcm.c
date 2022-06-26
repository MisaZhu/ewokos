// minimp3 example player application for Linux/OSS
// this file is public domain -- do with it whatever you want!
#include <unistd.h>
#include <sys/vfs.h>
#include <stdio.h>
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
		printf("Usage: %s [mp3_fname]\n", argv[0]);
		return 1;
	}

	file_data = vfs_readfile(argv[1], &bytes_left);
	if(file_data == NULL) {
		printf("Error: read MP3 audio file failed!\n");
		return 1;
	}
	printf("read MP3 audio file size: %d.\n", bytes_left);

	stream_pos = (unsigned char *) file_data;
	//bytes_left -= 100;

	mp3dec_init(&mp3);
	mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);
	if (info.frame_bytes == 0) {
		printf("Error: not a valid MP3 audio file!\n");
		free(file_data);
		return 1;
	}

	while ((bytes_left >= 0) && (info.frame_bytes > 0)) {
		stream_pos += info.frame_bytes;
		bytes_left -= info.frame_bytes;
		write(1, (const void *) sample_buf, info.frame_bytes);
		mp3dec_decode_frame(&mp3, stream_pos, bytes_left, sample_buf, &info);
	}
	free(file_data);
	return 0;
}
