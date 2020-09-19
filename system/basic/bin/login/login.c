#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/md5.h>
#include <sys/mstr.h>

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
} user_info_t;

static user_info_t _users[USER_NUM_MAX];
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
	strncpy(val, s->cstr, len);
	str_free(s);
	return 0;
}

static int read_user_item(int fd, user_info_t* info) {
	if(read_value(fd, info->user, USER_MAX, true) != 0)
		return -1;

	char id[32];
	if(read_value(fd, id, 32, false) != 0)
		return -1;
	info->gid = atoi(id);
	
	if(read_value(fd, id, 32, false) != 0)
		return -1;
	info->uid = atoi(id);

	if(read_value(fd, info->home, HOME_MAX, false) != 0)
		return -1;

	if(read_value(fd, info->cmd, CMD_MAX, false) != 0)
		return -1;

	if(read_value(fd, info->password, PSWD_MAX, false) != 0)
		return -1;
	return 0;
}

static int read_user_info(void) {
	int fd = open("/etc/password", O_RDONLY);
	if(fd < 0) {
		dprintf(3, "Error, open password file failed!\n");
		return -1;
	}
	_user_num = 0;

	while(_user_num < USER_NUM_MAX) {
		user_info_t* info = &_users[_user_num];
		if(read_user_item(fd, info) != 0)
			break;
		_user_num++;
	}
	close(fd);
	return 0;
}

static void input(str_t* s, bool show) {
	str_reset(s);
	char c;
	while(true) {
		if(read(0, &c , 1) != 1)
			break;
		if(c == '\r')
			c = '\n';

		if(show || c == '\n')
			write(1, &c, 1);
		if(c == '\n')
			break;
		str_addc(s, c);
	}
}

static user_info_t* check(const char* user, const char* password) {
	int i;
	for(i=0; i<_user_num; i++) {
		user_info_t* info = &_users[i];
		if(strcmp(info->user, user) == 0) {
			if(password[0] == 0)
				return info;
			const char* md5 = md5_encode_str((uint8_t*)password, strlen(password));
			if(strcmp(info->password, md5) == 0) 
				return info;
			else
				return NULL;
		}
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	if(read_user_info() != 0)
		return -1;

	str_t* user = str_new("");
	str_t* password = str_new("");

	printf("login: ");
	input(user, true);
	if(user->len > 0) {
		printf("password: ");
		input(password, false);
	}

	user_info_t* info = check(user->cstr, password->cstr);
	str_free(user);
	str_free(password);

	if(info == NULL || info->cmd[0] == 0)
		return -1;

	setenv("HOME", info->home);
	setuid(info->uid);
	exec(info->cmd);
	return 0;
}
