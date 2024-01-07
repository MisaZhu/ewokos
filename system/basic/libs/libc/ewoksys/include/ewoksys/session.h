#ifndef SESSION_H
#define SESSION_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	SESSION_CHECK = 0,
	SESSION_SET
};

#define USER_MAX 32
#define PSWD_MAX 64
#define HOME_MAX 64
#define CMD_MAX 64
#define USER_NUM_MAX 64

typedef struct  {
	char user[USER_MAX];
	int32_t gid;
	int32_t uid;
	char home[HOME_MAX];
	char cmd[CMD_MAX];
	char password[PSWD_MAX];
} session_info_t;

#define  IPC_SERV_SESSIOND "ipc_serv.sessiond"

int session_check(const char* name, const char* passwd, session_info_t* sinfo);
int session_set(session_info_t* sinfo);

#ifdef __cplusplus
}
#endif

#endif