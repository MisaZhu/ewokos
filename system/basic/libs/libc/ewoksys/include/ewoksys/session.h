#ifndef SESSION_H
#define SESSION_H

#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	SESSION_CHECK = 0,
	SESSION_GET_BY_UID,
	SESSION_GET_BY_NAME,
	SESSION_SET
};

#define SESSION_USER_MAX 32
#define SESSION_PSWD_MAX 64
#define SESSION_HOME_MAX 64
#define SESSION_CMD_MAX 64

typedef struct  {
	char user[SESSION_USER_MAX];
	int32_t gid;
	int32_t uid;
	char home[SESSION_HOME_MAX];
	char cmd[SESSION_CMD_MAX];
	char password[SESSION_PSWD_MAX];
} session_info_t;

#define  IPC_SERV_SESSIOND "ipc_serv.sessiond"

int session_check(const char* name, const char* passwd, session_info_t* sinfo);
int session_set(session_info_t* sinfo);
int session_get_by_uid(int32_t uid, session_info_t* sinfo);
int session_get_by_name(const char* name, session_info_t* sinfo);

#ifdef __cplusplus
}
#endif

#endif