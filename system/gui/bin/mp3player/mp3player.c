// minimp3 example player application for Linux/OSS
// this file is public domain -- do with it whatever you want!
#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3/mp3player.h"

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [mp3_fname]\n", argv[0]);
		return 1;
	}

	if(argc >= 3) {
		mp3_play_file(argv[1], argv[2]);
	} else {
		mp3_play_file(argv[1], "/dev/sound0");
	}
	return 0;
}
