#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <syscall.h>
#include <sconf.h>

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

static void gets(char* buf, int len) {
	int i = 0;
	while(true) {
		if(i >= len)
			break;

		int c = getch();
		if(c == 0) {
			sleep(0);
			continue;
		}

		if (c == KEY_BACKSPACE) {
			if (i > 0) {
				//delete last char
				putch(KEY_LEFT); 
				putch(' ');
				putch(KEY_LEFT); 
				i--;
			}
		}
		else if (c == 8) {
			if (i > 0) {
				//delete last char
				putch(c); 
				i--;
			}
		}
		else {
			putch(c);
			if(c == '\r' || c == '\n')
				break;
			buf[i] = c;
			i++;
		}
	}
	buf[i] = 0;
}

static int cd(const char* dir) {
	char cwd[FULL_NAME_MAX];
	if(getcwd(cwd, FULL_NAME_MAX-1) == NULL)
		return -1;

	if(strcmp(dir, ".") == 0)
		return 0;

	while(*dir == ' ') /*skip all space*/
		dir++;

	if(dir[0] == 0) {
		chdir("/");
		return 0;
	}

	if(strcmp(dir, "..") == 0) {
		if(strcmp(cwd, "/") == 0)
			return 0;

		int len = strlen(cwd)  - 1;
		for(int i=len; i>=0; i--) {
			if(cwd[i] == '/') {
				cwd[i] = 0;
				break;
			}
		}
		if(cwd[0] == 0) {
			chdir("/");
			return 0;
		}
	}
	else if(dir[0] == '/') {
		strncpy(cwd, dir, FULL_NAME_MAX);
	}
	else {
		int len = strlen(cwd);
		if(cwd[len-1] != '/') {
			cwd[len] = '/';
			len++;
		}
		strcpy(cwd+len, dir);
	}

	fs_info_t info;
	if(fs_finfo(cwd, &info) != 0)
		printf("[%s] not exist!\n", dir);	
	else if(info.type != FS_TYPE_DIR)
		printf("[%s] is not a directory!\n", dir);	
	else 
		chdir(cwd);
	return 0;
}

void export_all() {
	int32_t i=0;
	while(true) {
		char name[64], value[128];
		if(syscall3(SYSCALL_GET_ENV_NAME, i, (int32_t)name, 63) != 0 || name[0] == 0)
			break;
		if(syscall3(SYSCALL_GET_ENV_VALUE, i, (int32_t)value, 127) != 0)
			break;
		printf("%s=%s\n", name, value);
		i++;
	}
}

void export_get(const char* arg) {
	char value[128];
	if(syscall3(SYSCALL_GET_ENV, (int32_t)arg, (int32_t)value, 127) != 0) 
		return;
	printf("%s=%s\n", arg, value);
}

void export_set(const char* arg) {
	char name[64];
	char* v = strchr(arg, '=');
	if(v == NULL)
		return;
	strncpy(name, arg, v-arg);

	syscall2(SYSCALL_SET_ENV, (int32_t)name, (int32_t)(v+1));
}

int export(const char* arg) {
	char* v = strchr(arg, '=');
	if(v == NULL)
		export_get(arg);
	else
		export_set(arg);
	return 0;
}

static int handle(const char* cmd) {
	if(strcmp(cmd, "cd") == 0) {
		return cd("/");
	}
	else if(strncmp(cmd, "cd ", 3) == 0) {
		return cd(cmd + 3);
	}
	else if(strcmp(cmd, "export") == 0) {
		export_all();
		return 0;
	}
	else if(strncmp(cmd, "export ", 7) == 0) {
		return export(cmd+7);
	}
	return -1; /*not shell internal command*/
}

static int32_t find_exec(char* fname, char* cmd) {
	fname[0] = 0;
	fs_info_t info;
	int32_t i = 0;	
	int32_t at = -1;
	char c = 0;
	while(cmd[i] != 0) {
		if(cmd[i] == ' ') {
			c = ' ';
			cmd[i] = 0;
			break;
		}
		i++;
	}
	at = i;

	if(cmd[0] == '/') {
		strcpy(fname, cmd);
		if(fs_finfo(fname, &info) == 0) {
			cmd[at] = c;
			strcpy(fname, cmd);
			return 0;
		}
	}

	const char* paths = getenv("PATH");
	char path[FULL_NAME_MAX];
	i = 0;
	while(true) {
		if(paths[i] == 0 || paths[i] == ':') {
			strncpy(path, paths, i);
			if(path[0] != 0) {
				snprintf(fname, FULL_NAME_MAX-1, "%s/%s", path, cmd);
				if(fs_finfo(fname, &info) == 0) {
					cmd[at] = c;
					snprintf(fname, FULL_NAME_MAX-1, "%s/%s", path, cmd);
					return 0;
				}
			}
			if(paths[i] == 0)
				break;
			paths = paths+i+1;
			fname[0] = 0;
		}
		++i;
	}

	cmd[at] = c;
	return -1;
}

static int32_t read_config() {
	sconf_t *conf = sconf_load("/etc/shell.conf");	
	if(conf == NULL)
		return -1;
	const char* v = sconf_get(conf, "PATH");
	if(v[0] != 0) 
		setenv("PATH", v);
	sconf_free(conf, free);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	char cmd[CMD_MAX];
	char cwd[FULL_NAME_MAX];

	setenv("PATH", "/bin");
	read_config();

	int uid = getuid();
	while(1) {
		if(uid == 0)
			printf("ewok:%s.# ", getcwd(cwd, FULL_NAME_MAX));
		else
			printf("ewok:%s.$ ", getcwd(cwd, FULL_NAME_MAX));

		gets(cmd, CMD_MAX);
		if(cmd[0] == 0)
			continue;

		if(strcmp(cmd, "exit") == 0)
			break;

		if(handle(cmd) == 0)
			continue;

		int len = strlen(cmd);
		len -= 1;
		bool fg = true;
		if(cmd[len] == '&') {
			cmd[len] = 0;
			fg = false;
		}	

		char fname[FULL_NAME_MAX];
		if(find_exec(fname, cmd) != 0) {
			printf("'%s' not found!\n", cmd);
			continue;
		}

		int child_pid = fork();
		if (child_pid == 0) {
			exec(fname);
			return 0;
		}
		else if(fg) {
			wait(child_pid);
		}
		cmd[0] = 0;
	}
	return 0;
}
