#ifndef XGO_H
#define XGO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t DATA_MAX = 8;
static const uint32_t CMD_SEND = 0x0;
static const uint32_t CMD_READ = 0x02;

extern int  xgo_init(void);
extern void xgo_quit(void);
extern int  xgo_send(uint8_t type, uint8_t cmd,uint8_t data);
extern int  xgo_cmd(uint8_t type, uint8_t cmd,uint8_t data, uint8_t res[DATA_MAX]);

#ifdef __cplusplus
}
#endif

#endif