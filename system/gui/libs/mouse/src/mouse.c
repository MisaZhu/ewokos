#include <mouse/mouse.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int mouse_read(int fd, mouse_evt_t* evt) {
	return read(fd, evt, sizeof(mouse_evt_t)) == sizeof(mouse_evt_t);
}

#ifdef __cplusplus
}
#endif