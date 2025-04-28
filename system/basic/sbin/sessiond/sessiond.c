#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ewoksys/md5.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/klog.h>

#define USER_NUM_MAX 64

static session_info_t _users[USER_NUM_MAX];
static int _user_num = 0;

static char skip_space(int fd) {
	char c;
	while(true) {
		if(read(fd, &c , 1) != 1)
			break;
		if(c != ' ' && c != '\t' && c != '\r' &&  c != '\n')
			return c;
	}
	return 0;
}

static int read_value(int fd, char* val, uint32_t len, bool start) {
	str_t* s = str_new("");
	char c = 0;
	if(start) {
		c = skip_space(fd);
		if(c == 0)
			return -1;
		str_addc(s, c);
	}

	while(true) {
		if(read(fd, &c , 1) != 1) {
			break;
		}
		if(c == ':' || c == '\r' || c == '\n')
			break;

		str_addc(s, c);
	}
	memset(val, 0, len);
	strncpy(val, s->cstr, len);
	str_free(s);
	return 0;
}

static int read_user_item(int fd, session_info_t* info) {
	if(read_value(fd, info->user, SESSION_USER_MAX, true) != 0)
		return -1;

	char id[32];
	if(read_value(fd, id, 32, false) != 0)
		return -1;
	info->gid = atoi(id);
	
	if(read_value(fd, id, 32, false) != 0)
		return -1;
	info->uid = atoi(id);

	if(read_value(fd, info->home, SESSION_HOME_MAX, false) != 0)
		return -1;

	if(read_value(fd, info->cmd, SESSION_CMD_MAX, false) != 0)
		return -1;

	if(read_value(fd, info->password, SESSION_PSWD_MAX, false) != 0)
		return -1;
	return 0;
}

static int read_user_info(void) {
	int fd = open("/etc/passwd", O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "Error, open password file failed!\n");
		return -1;
	}
	_user_num = 0;

	while(_user_num < USER_NUM_MAX) {
		session_info_t* info = &_users[_user_num];
		if(read_user_item(fd, info) != 0)
			break;
		_user_num++;
	}
	close(fd);
	return 0;
}

static session_info_t* check(const char* user, const char* password, int* res) {
	int i;
	*res = 0;
	for(i=0; i<_user_num; i++) {
		session_info_t* info = &_users[i];
		if(strcmp(info->user, user) == 0) {
			if(info->password[0] == 0)
				return info;
			const char* md5 = md5_encode_str((uint8_t*)password, strlen(password));
			if(strcmp(info->password, md5) == 0) 
				return info;
			else {
				*res = SESSION_ERR_PWD; //Wrong password
				return NULL;
			}
		}
	}
	*res = SESSION_ERR_USR; //user not existed
	return NULL;
}

static session_info_t* get_by_name(const char* user) {
	int i;
	for(i=0; i<_user_num; i++) {
		session_info_t* info = &_users[i];
		if(strcmp(info->user, user) == 0) {
			return info;
		}
	}
	return NULL;
}

static session_info_t* get_by_uid(int32_t uid) {
	int i;
	for(i=0; i<_user_num; i++) {
		session_info_t* info = &_users[i];
		if(info->uid == uid) {
			return info;
		}
	}
	return NULL;
}

static session_info_t* secure_session(const session_info_t* sinfo) {
	static session_info_t to;
	memcpy(&to, sinfo, sizeof(session_info_t));
	memset(to.password, 0, SESSION_PSWD_MAX);
	return &to;
}

static void do_session_get_by_uid(int pid, proto_t* in, proto_t* out) {
	int32_t uid = proto_read_int(in);
	session_info_t* sinfo = get_by_uid(uid);

	PF->clear(out)->addi(out, -1);
	if(sinfo == NULL)
			return;
	sinfo = secure_session(sinfo);
	PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void do_session_get_by_name(int pid, proto_t* in, proto_t* out) {
	const char* name = proto_read_str(in);
	session_info_t* sinfo = get_by_name(name);

	PF->clear(out)->addi(out, -1);
	if(sinfo == NULL)
			return;
	sinfo = secure_session(sinfo);
	PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void do_session_check(int pid, proto_t* in, proto_t* out) {
	const char* name = proto_read_str(in);
	const char* passwd = proto_read_str(in);
	int res = 0;
	session_info_t* sinfo = check(name, passwd, &res);
	PF->clear(out)->addi(out, res);
	if(sinfo == NULL)
			return;

	sinfo = secure_session(sinfo);
	PF->clear(out)->addi(out, 0)->add(out, sinfo, sizeof(session_info_t));
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)p;
	pid = proc_getpid(pid);

	switch(cmd) {
	case SESSION_CHECK: 
		do_session_check(pid, in, out);
		return;
	case SESSION_GET_BY_UID: 
		do_session_get_by_uid(pid, in, out);
		return;
	case SESSION_GET_BY_NAME: 
		do_session_get_by_name(pid, in, out);
		return;
	}
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	if(read_user_info() != 0)
		return -1;

	if(ipc_serv_reg(IPC_SERV_SESSIOND) != 0) {
		klog("reg sessiond ipc_serv error!\n");
		return -1;
	}

	ipc_serv_run(handle_ipc, NULL, NULL, 0);

	while(1) {
		proc_block_by(getpid(), (uint32_t)main);
	}
	return 0;
}

