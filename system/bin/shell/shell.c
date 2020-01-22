#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <vprintf.h>
#include <mstr.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dev/device.h>
#include <sys/wait.h>

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

static int32_t gets(str_t* buf) {
	str_reset(buf);	
	while(1) {
		int c = getch();
		if(c == 0)
			return -1;

		if (c == KEY_BACKSPACE) {
			if (buf->len > 0) {
				//delete last char
				putch(KEY_LEFT); 
				putch(' ');
				putch(KEY_LEFT); 
				buf->len--;
			}
		}
		else if (c == 8) {
			if (buf->len > 0) {
				//delete last char
				putch(c); 
				buf->len--;
			}
		}
		else {
			putch(c);
			if(c == '\r' || c == '\n')
				break;
			str_addc(buf, c);
		}
		usleep(10000);
	}
	str_addc(buf, 0);
	return 0;
}

static int cd(const char* dir) {
	char cwd[FS_FULL_NAME_MAX];
	if(getcwd(cwd, FS_FULL_NAME_MAX-1) == NULL)
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
		strcpy(cwd, dir);
	}
	else {
		int len = strlen(cwd);
		if(cwd[len-1] != '/') {
			cwd[len] = '/';
			len++;
		}
		strcpy(cwd+len, dir);
	}

	fsinfo_t info;
	if(vfs_get(cwd, &info) != 0)
		printf("[%s] not exist!\n", dir);	
	else if(info.type != FS_TYPE_DIR)
		printf("[%s] is not a directory!\n", dir);	
	else 
		chdir(cwd);
	return 0;
}


static void export_all(void) {
	char name[64], value[1024];
	int32_t i=0;
	while(1) {
		if(syscall3(SYS_PROC_GET_ENV_NAME, i, (int32_t)name, 63) != 0 || name[0] == 0)
			break;
		if(syscall3(SYS_PROC_GET_ENV_VALUE, i, (int32_t)value, 1023) != 0)
			break;
		printf("declare -x %s=%s\n", name, value);
		i++;
	}
}

static void export_get(const char* arg) {
	char value[1024];
	if(syscall3(SYS_PROC_GET_ENV, (int32_t)arg, (int32_t)value, 127) != 0) 
		return;
	printf("%s=%s\n", arg, value);
}

static void export_set(const char* arg) {
	char name[64];
	char* v = strchr(arg, '=');
	if(v == NULL)
		return;
	strncpy(name, arg, v-arg);
	name[v-arg] = 0;

	syscall2(SYS_PROC_SET_ENV, (int32_t)name, (int32_t)(v+1));
}

static int export(const char* arg) {
	char* v = strchr(arg, '=');
	if(v == NULL)
		export_get(arg);
	else
		export_set(arg);
	return 0;
}

static int _terminated = 0;

static int handle(const char* cmd) {
	if(strcmp(cmd, "exit") == 0) {
		_terminated = 1;
		return 0;
	}
	else if(strcmp(cmd, "cd") == 0) {
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

#define ENV_PATH "PATH"

static int32_t find_exec(char* fname, char* cmd) {
	fname[0] = 0;
	fsinfo_t info;
	//get the cmd file name(without arguments).
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
	//if cmd file is fullpath.
	if(cmd[0] == '/') {
		strcpy(fname, cmd);
		if(vfs_get(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
			cmd[at] = c;
			strcpy(fname, cmd);
			return 0;
		}
	}
	//search executable file in PATH dirs.
	const char* paths = getenv(ENV_PATH);
	char path[FS_FULL_NAME_MAX];
	i = 0;
	while(1) {
		if(paths[i] == 0 || paths[i] == ':') {
			strncpy(path, paths, i);
			path[i] = 0;
			if(path[0] != 0) {
				snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
				if(vfs_get(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
					cmd[at] = c;
					snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
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

static void redir(const char* fname, int in) {
	while(*fname == ' ')
		fname++;

	if(in != 0) {
		int32_t fd = open(fname, O_RDONLY);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 0);
		close(fd);
	}
	else {
		int32_t fd = open(fname, O_WRONLY | O_CREAT);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 1);
		close(fd);
	}
}

static int do_cmd(char* cmd) {
	while(*cmd == ' ')
		cmd++;

	char fname[FS_FULL_NAME_MAX];
	if(find_exec(fname, cmd) != 0) {
		printf("'%s' not found!\n", cmd);
		return -1;
	}
	exec(fname);
	return 0;
}

static int run_cmd(char* cmd);
static int do_pipe_cmd(char* p1, char* p2) {
	int fds[2];
	if(pipe(fds) != 0) {
		printf("pipe create failed!\n");
		return -1;
	}

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		close(fds[1]);
		dup2(fds[0], 0);
		close(fds[0]);
		run_cmd(p2);
		exit(0);
	}
	//child proc for p1 writer
	close(fds[0]);
	dup2(fds[1], 1);
	close(fds[1]);
	return do_cmd(p1);
}

static int run_cmd(char* cmd) {
	char* proc = NULL;
	while(*cmd != 0) {
		char c = *cmd++;
		if(proc == NULL && c == ' ')
			continue;

		if(c == '>') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 0); //redir OUT.
			return do_cmd(proc);
		}
		else if(c == '<') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 1); //redir in.
			return do_cmd(proc);
		}
		else if(c == '|') { //pipe
			*(cmd-1) = 0;	
			return do_pipe_cmd(proc, cmd);
		}
		else if(proc == NULL)
			proc = cmd-1;
	}

	if(proc != NULL)
		do_cmd(proc);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* cid = getenv("CONSOLE_ID");
	if(cid[0] == 0)
		cid = "0";

	str_t* cmdstr = str_new("");
	_terminated = 0;
	while(_terminated == 0) {
		char cwd[FS_FULL_NAME_MAX+1];
		printf("ewok(%s):%s# ", cid, getcwd(cwd, FS_FULL_NAME_MAX));

		if(gets(cmdstr) != 0)
			break;

		char* cmd = cmdstr->cstr;
		if(cmd[0] == 0)
			continue;
		if(handle(cmd) == 0)
			continue;

		int len = strlen(cmd)-1;
		int fg = 1;
		if(cmd[len] == '&') {
			cmd[len] = 0;
			fg = 0;
		}	

		int child_pid = fork();
		if (child_pid == 0) {
			if(fg == 0)
				detach();
			int res = run_cmd(cmd);
			str_free(cmdstr);	
			return res;
		}
		else if(fg != 0) {
			waitpid(child_pid);
		}
	}

	str_free(cmdstr);	
	return 0;
}
