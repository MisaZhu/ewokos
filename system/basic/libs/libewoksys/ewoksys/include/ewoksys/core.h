#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif


enum {
	CORE_CMD_IPC_SERV_REG = 0,
	CORE_CMD_IPC_SERV_UNREG,
	CORE_CMD_IPC_SERV_GET,
	CORE_CMD_SET_CWD,
	CORE_CMD_GET_CWD,
	CORE_CMD_SET_ENV,
	CORE_CMD_GET_ENV,
	CORE_CMD_GET_ENVS,
	CORE_CMD_CLONE
};

void     schd_core_lock(void); 
void     schd_core_unlock(void); 

#define neon_lock schd_core_lock
#define neon_unlock schd_core_unlock

#ifdef __cplusplus
}
#endif

#endif
