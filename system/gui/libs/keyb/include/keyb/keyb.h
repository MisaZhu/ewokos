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

typedef struct {
	uint8_t key;
	uint8_t state;
} keyb_evt_t;

#define KEYB_EVT_MAX  6

int keyb_read(int keyb_fd, keyb_evt_t* evts, uint8_t num);

#ifdef __cplusplus
}
#endif

#endif