#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t nes_key_state(uint8_t b);
uint8_t nes_key_state_ctrl2(uint8_t b);
uint8_t psg_io_read(void);
void psg_io_write(uint8_t data);
uint8_t psg_io_read2(void);
void psg_io_write2(uint8_t data);
#ifdef __cplusplus
}
#endif
#endif
