#ifndef BCM_BOARD_H
#define BCM_BOARD_H

#include <stdint.h>

enum {
    PI_0 = 0,
    PI_0_W,
    PI_0_2W,
    PI_1A,
    PI_1B,
    PI_2B,
    PI_3A,
    PI_3B,
    PI_4B_1G,
    PI_4B_2G,
    PI_4B_4G,
    PI_4B_8G,
    PI_CM3_1G,
    PI_CM4_1G,
    PI_CM4_2G,
    PI_CM4_4G,
    PI_CM4_8G,
	PI_5_2G,
	PI_5_4G,
	PI_5_8G,
	PI_5_16G,
	PI_CM5_2G,
	PI_CM5_4G,
	PI_CM5_8G,
	PI_500,
	PI_CM5_LITE_2G,
	PI_CM5_LITE_4G,
	PI_CM5_LITE_8G,
    PI_UNKNOWN
};

uint32_t bcm283x_board(void);

#endif
