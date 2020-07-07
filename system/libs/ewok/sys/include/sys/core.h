#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif


enum {
	CORE_CMD_GLOBAL_SET = 0,
	CORE_CMD_GLOBAL_GET,
	CORE_CMD_GLOBAL_DEL,
	CORE_CMD_IPC_SERV_REG,
	CORE_CMD_IPC_SERV_UNREG,
	CORE_CMD_IPC_SERV_GET,
};

#ifdef __cplusplus
}
#endif

#endif
