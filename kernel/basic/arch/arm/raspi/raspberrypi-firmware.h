/*
 *  Copyright © 2015 Broadcom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOC_RASPBERRY_FIRMWARE_H__
#define __SOC_RASPBERRY_FIRMWARE_H__


const uint32_t 	RPI_FIRMWARE_STATUS_REQUEST = 0;
const uint32_t 	RPI_FIRMWARE_STATUS_SUCCESS = 0x80000000;
const uint32_t 	RPI_FIRMWARE_STATUS_ERROR =   0x80000001;

/**
 * struct rpi_firmware_property_tag_header - Firmware property tag header
 * @tag:		One of enum_mbox_property_tag.
 * @buf_size:		The number of bytes in the value buffer following this
 *			struct.
 * @req_resp_size:	On submit, the length of the request (though it doesn't
 *			appear to be currently used by the firmware).  On return,
 *			the length of the response (always 4 byte aligned), with
 *			the low bit set.
 */
struct rpi_firmware_property_tag_header {
	uint32_t tag;
	uint32_t buf_size;
	uint32_t req_resp_size;
};

const uint32_t 	RPI_FIRMWARE_PROPERTY_END =                           0;
const uint32_t 	RPI_FIRMWARE_GET_FIRMWARE_REVISION =                  0x00000001;
const uint32_t 	RPI_FIRMWARE_GET_FIRMWARE_VARIANT =                   0x00000002;
const uint32_t 	RPI_FIRMWARE_GET_FIRMWARE_HASH =                      0x00000003;
const uint32_t 	RPI_FIRMWARE_SET_CURSOR_INFO =                        0x00008010;
const uint32_t 	RPI_FIRMWARE_SET_CURSOR_STATE =                       0x00008011;
const uint32_t 	RPI_FIRMWARE_GET_BOARD_MODEL =                        0x00010001;
const uint32_t 	RPI_FIRMWARE_GET_BOARD_REVISION =                     0x00010002;
const uint32_t 	RPI_FIRMWARE_GET_BOARD_MAC_ADDRESS =                  0x00010003;
const uint32_t 	RPI_FIRMWARE_GET_BOARD_SERIAL =                       0x00010004;
const uint32_t 	RPI_FIRMWARE_GET_ARM_MEMORY =                         0x00010005;
const uint32_t 	RPI_FIRMWARE_GET_VC_MEMORY =                          0x00010006;
const uint32_t 	RPI_FIRMWARE_GET_CLOCKS =                             0x00010007;
const uint32_t 	RPI_FIRMWARE_GET_POWER_STATE =                        0x00020001;
const uint32_t 	RPI_FIRMWARE_GET_TIMING =                             0x00020002;
const uint32_t 	RPI_FIRMWARE_SET_POWER_STATE =                        0x00028001;
const uint32_t 	RPI_FIRMWARE_GET_CLOCK_STATE =                        0x00030001;
const uint32_t 	RPI_FIRMWARE_GET_CLOCK_RATE =                         0x00030002;
const uint32_t 	RPI_FIRMWARE_GET_VOLTAGE =                            0x00030003;
const uint32_t 	RPI_FIRMWARE_GET_MAX_CLOCK_RATE =                     0x00030004;
const uint32_t 	RPI_FIRMWARE_GET_MAX_VOLTAGE =                        0x00030005;
const uint32_t 	RPI_FIRMWARE_GET_TEMPERATURE =                        0x00030006;
const uint32_t 	RPI_FIRMWARE_GET_MIN_CLOCK_RATE =                     0x00030007;
const uint32_t 	RPI_FIRMWARE_GET_MIN_VOLTAGE =                        0x00030008;
const uint32_t 	RPI_FIRMWARE_GET_TURBO =                              0x00030009;
const uint32_t 	RPI_FIRMWARE_GET_MAX_TEMPERATURE =                    0x0003000a;
const uint32_t 	RPI_FIRMWARE_GET_STC =                                0x0003000b;
const uint32_t 	RPI_FIRMWARE_ALLOCATE_MEMORY =                        0x0003000c;
const uint32_t 	RPI_FIRMWARE_LOCK_MEMORY =                            0x0003000d;
const uint32_t 	RPI_FIRMWARE_UNLOCK_MEMORY =                          0x0003000e;
const uint32_t 	RPI_FIRMWARE_RELEASE_MEMORY =                         0x0003000f;
const uint32_t 	RPI_FIRMWARE_EXECUTE_CODE =                           0x00030010;
const uint32_t 	RPI_FIRMWARE_EXECUTE_QPU =                            0x00030011;
const uint32_t 	RPI_FIRMWARE_SET_ENABLE_QPU =                         0x00030012;
const uint32_t 	RPI_FIRMWARE_GET_DISPMANX_RESOURCE_MEM_HANDLE =       0x00030014;
const uint32_t 	RPI_FIRMWARE_GET_EDID_BLOCK =                         0x00030020;
const uint32_t 	RPI_FIRMWARE_GET_CUSTOMER_OTP =                       0x00030021;
const uint32_t 	RPI_FIRMWARE_GET_EDID_BLOCK_DISPLAY =                 0x00030023;
const uint32_t 	RPI_FIRMWARE_GET_DOMAIN_STATE =                       0x00030030;
const uint32_t 	RPI_FIRMWARE_GET_THROTTLED =                          0x00030046;
const uint32_t 	RPI_FIRMWARE_GET_CLOCK_MEASURED =                     0x00030047;
const uint32_t 	RPI_FIRMWARE_NOTIFY_REBOOT =                          0x00030048;
const uint32_t 	RPI_FIRMWARE_SET_CLOCK_STATE =                        0x00038001;
const uint32_t 	RPI_FIRMWARE_SET_CLOCK_RATE =                         0x00038002;
const uint32_t 	RPI_FIRMWARE_SET_VOLTAGE =                            0x00038003;
const uint32_t 	RPI_FIRMWARE_SET_TURBO =                              0x00038009;
const uint32_t 	RPI_FIRMWARE_SET_CUSTOMER_OTP =                       0x00038021;
const uint32_t 	RPI_FIRMWARE_SET_DOMAIN_STATE =                       0x00038030;
const uint32_t 	RPI_FIRMWARE_GET_GPIO_STATE =                         0x00030041;
const uint32_t 	RPI_FIRMWARE_SET_GPIO_STATE =                         0x00038041;
const uint32_t 	RPI_FIRMWARE_SET_SDHOST_CLOCK =                       0x00038042;
const uint32_t 	RPI_FIRMWARE_GET_GPIO_CONFIG =                        0x00030043;
const uint32_t 	RPI_FIRMWARE_SET_GPIO_CONFIG =                        0x00038043;
const uint32_t 	RPI_FIRMWARE_GET_PERIPH_REG =                         0x00030045;
const uint32_t 	RPI_FIRMWARE_SET_PERIPH_REG =                         0x00038045;
const uint32_t 	RPI_FIRMWARE_GET_POE_HAT_VAL =                        0x00030049;
const uint32_t 	RPI_FIRMWARE_SET_POE_HAT_VAL =                        0x00030050;
/* Dispmanx TAGS */
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_ALLOCATE =                   0x00040001;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_BLANK =                      0x00040002;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_PHYSICAL_WIDTH_HEIGHT =  0x00040003;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_VIRTUAL_WIDTH_HEIGHT =   0x00040004;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_DEPTH =                  0x00040005;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_PIXEL_ORDER =            0x00040006;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_ALPHA_MODE =             0x00040007;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_PITCH =                  0x00040008;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_VIRTUAL_OFFSET =         0x00040009;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_OVERSCAN =               0x0004000a;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_PALETTE =                0x0004000b;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_LAYER =                  0x0004000c;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_TRANSFORM =              0x0004000d;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_VSYNC =                  0x0004000e;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_TOUCHBUF =               0x0004000f;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_GPIOVIRTBUF =            0x00040010;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_RELEASE =                    0x00048001;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_DISPLAY_ID =             0x00040016;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_DISPLAY_NUM =            0x00048013;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_NUM_DISPLAYS =           0x00040013;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_GET_DISPLAY_SETTINGS =       0x00040014;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_PHYSICAL_WIDTH_HEIGHT = 0x00044003;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_VIRTUAL_WIDTH_HEIGHT =  0x00044004;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_DEPTH =                 0x00044005;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_PIXEL_ORDER =           0x00044006;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_ALPHA_MODE =            0x00044007;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_VIRTUAL_OFFSET =        0x00044009;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_OVERSCAN =              0x0004400a;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_PALETTE =               0x0004400b;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_LAYER =                 0x0004400c;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_TRANSFORM =             0x0004400d;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_TEST_VSYNC =                 0x0004400e;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_PHYSICAL_WIDTH_HEIGHT =  0x00048003;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_WIDTH_HEIGHT =   0x00048004;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_DEPTH =                  0x00048005;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_PIXEL_ORDER =            0x00048006;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_ALPHA_MODE =             0x00048007;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_OFFSET =         0x00048009;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_OVERSCAN =               0x0004800a;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_PALETTE =                0x0004800b;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_TOUCHBUF =               0x0004801f;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_GPIOVIRTBUF =            0x00048020;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_VSYNC =                  0x0004800e;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_LAYER =                  0x0004800c;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_TRANSFORM =              0x0004800d;
const uint32_t 	RPI_FIRMWARE_FRAMEBUFFER_SET_BACKLIGHT =              0x0004800f;
const uint32_t 	RPI_FIRMWARE_VCHIQ_INIT =                             0x00048010;
const uint32_t 	RPI_FIRMWARE_SET_PLANE =                              0x00048015;
const uint32_t 	RPI_FIRMWARE_GET_DISPLAY_TIMING =                     0x00040017;
const uint32_t 	RPI_FIRMWARE_SET_TIMING =                             0x00048017;
const uint32_t 	RPI_FIRMWARE_GET_DISPLAY_CFG =                        0x00040018;
const uint32_t 	RPI_FIRMWARE_SET_DISPLAY_POWER =		      		  0x00048019;
const uint32_t 	RPI_FIRMWARE_GET_COMMAND_LINE =                       0x00050001;
const uint32_t 	RPI_FIRMWARE_GET_DMA_CHANNELS =                       0x00060001;
#endif /* __SOC_RASPBERRY_FIRMWARE_H__ */
