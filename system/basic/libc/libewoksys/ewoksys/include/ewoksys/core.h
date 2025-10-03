#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum {
	CORE_CMD_IPC_SERV_REG = 0,
	CORE_CMD_IPC_SERV_UNREG,
	CORE_CMD_IPC_SERV_GET,
	CORE_CMD_SET_CWD,
	CORE_CMD_GET_CWD,
	CORE_CMD_SET_ENV,
	CORE_CMD_GET_ENV,
	CORE_CMD_GET_ENVS,
	CORE_CMD_CLONE,

	CORE_CMD_ENABLE_UX,
	CORE_CMD_SET_ACTIVE_UX,
	CORE_CMD_NEXT_UX,
	CORE_CMD_PREV_UX,
	CORE_CMD_GET_UX
};

#define UX_X_DEFAULT 7
#define UX_MAX 8

int      core_next_ux(int32_t disp_index);
int      core_prev_ux(int32_t disp_index);
int      core_set_active_ux(int32_t disp_index, int ux_index);
int      core_get_active_ux(int32_t disp_index);
int      core_get_ux_env(void);
int      core_enable_ux(int32_t disp_index, int ux_index);

#ifdef __cplusplus
}
#endif

#endif
