#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <types.h>

uint8_t* brcmf_fw_get_firmware(uint32_t* len);
uint8_t* brcmf_fw_get_nvram(uint32_t* len);
uint8_t* brcmf_fw_get_clm(uint32_t* len);

#endif