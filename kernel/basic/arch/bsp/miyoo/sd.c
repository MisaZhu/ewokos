#include <dev/sd.h>

// static IPEmType ge_IPSlot[3]     = {D_SDMMC1_IP, D_SDMMC2_IP, D_SDMMC3_IP};
// static PortEmType ge_PORTSlot[3] = {D_SDMMC1_PORT, D_SDMMC2_PORT, D_SDMMC3_PORT};
// static PADEmType  ge_PADSlot[3]  = {D_SDMMC1_PAD, D_SDMMC2_PAD, D_SDMMC3_PAD};
// static U32_T  gu32_MaxClkSlot[3] = {V_SDMMC1_MAX_CLK, V_SDMMC2_MAX_CLK, V_SDMMC3_MAX_CLK};

int32_t sd_init(void) {
	//printf("%s:%d\n", __func__, __LINE__);
	return -1;
}

int32_t sd_dev_read(int32_t sector) {
	//printf("%s:%d\n", __func__, __LINE__);
	return -1;
}

int32_t sd_dev_read_done(void* buf) {
	//printf("%s:%d\n", __func__, __LINE__);
	return -1;
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	//printf("%s:%d\n", __func__, __LINE__);
	return -1;
}

int32_t sd_dev_write_done(void) {
	//printf("%s:%d\n", __func__, __LINE__);
	return -1;
}
