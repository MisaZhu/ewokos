#include <mouse/mouse.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int mouse_read(int fd, mouse_evt_t* evt) {
	if(core_get_active_ux(0) != core_get_ux_env())
        return 0;

	return read(fd, evt, sizeof(mouse_evt_t)) == sizeof(mouse_evt_t);
}

#ifdef __cplusplus
}
#endif