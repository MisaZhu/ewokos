#ifndef KEYB_H
#define KEYB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	KEYB_STATE_PRESS = 0,
	KEYB_STATE_RELEASE
};

typedef void(*key_evt_handle_t)(uint8_t key, uint8_t state, void* p);

void keyb_read(int keyb_fd, key_evt_handle_t handle_func, void* p);

#ifdef __cplusplus
}
#endif

#endif