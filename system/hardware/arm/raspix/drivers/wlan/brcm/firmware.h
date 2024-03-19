#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <types.h>

uint8_t* brcmf_fw_get_firmware(int* len);
uint8_t* brcmf_fw_get_nvram(int* len);
uint8_t* brcmf_fw_get_clm(int* len);

#endif