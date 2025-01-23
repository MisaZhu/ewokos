#ifndef XGO_H
#define XGO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint8_t XGO_DATA_MAX    = 16;
static const uint8_t XGO_TYPE_SEND   = 0x0;
static const uint8_t XGO_TYPE_READ   = 0x02;

static const uint8_t XGO_CMD_GET_BATT        = 0x01;
static const uint8_t XGO_CMD_GET_POS         = 0x02;
static const uint8_t XGO_CMD_DEMO            = 0x03;
static const uint8_t XGO_CMD_SET_FORCE_RT    = 0x0A;
static const uint8_t XGO_CMD_SET_FORCE_ROLL  = 0x61;
static const uint8_t XGO_CMD_SET_MOVE_SPD    = 0x30;
static const uint8_t XGO_CMD_SET_ROTATE_SPD  = 0x32;
static const uint8_t XGO_CMD_SET_LEAN        = 0x33;
static const uint8_t XGO_CMD_SET_HEIGHT      = 0x35;
static const uint8_t XGO_CMD_SET_LEG_R       = 0x73;
static const uint8_t XGO_CMD_SET_LEG_L       = 0x74;
static const uint8_t XGO_CMD_ACT             = 0x3E;

static const uint8_t XGO_ACT_STOP            = 0x00;
static const uint8_t XGO_ACT_SHAKE           = 0x01;
static const uint8_t XGO_ACT_HEIGHT          = 0x02;
static const uint8_t XGO_ACT_MOVE            = 0x03;
static const uint8_t XGO_ACT_SNAKE           = 0x04;
static const uint8_t XGO_ACT_ROUND_H         = 0x05;
static const uint8_t XGO_ACT_ROUND_SH        = 0x06;
static const uint8_t XGO_ACT_MAX             = 0x07;

extern int  xgo_init(void);
extern void xgo_quit(void);
extern int  xgo_cmd(uint8_t type, uint8_t cmd, uint8_t data, uint8_t *res);

#ifdef __cplusplus
}
#endif

#endif