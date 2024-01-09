#include <ewoksys/session.h>
#include <ewoksys/ipc.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int get_sessiond_pid(void) {
	return ipc_serv_get(IPC_SERV_SESSIOND);
}

int session_check(const char* name, const char* passwd, session_info_t* sinfo) {
	int pid = get_sessiond_pid();
	if(pid < 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
			adds(&in, name)->
			adds(&in, passwd);

	int res = -1;
	if(ipc_call(pid, SESSION_CHECK, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(res == 0) {
			proto_read_to(&out, sinfo, sizeof(session_info_t));
		}
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int session_get(int32_t uid, session_info_t* sinfo) {
	memset(sinfo, 0, sizeof(session_info_t));
	int pid = get_sessiond_pid();
	if(pid < 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
			addi(&in, uid);

	int res = -1;
	if(ipc_call(pid, SESSION_GET, &in, &out) == 0) {
		res = proto_read_int(&out);
		if(res == 0) {
			proto_read_to(&out, sinfo, sizeof(session_info_t));
		}
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int session_set(session_info_t* sinfo) {
	(void)sinfo;
	return 0;
}


#ifdef __cplusplus
}
#endif