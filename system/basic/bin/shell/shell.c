#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>

#include <ewoksys/mstr.h>
#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/wait.h>
#include <ewoksys/keydef.h>
#include <sys/errno.h>
#include <setenv.h>
#include "shell.h"

bool _script_mode = false;
bool _terminated = false;

old_cmd_t* _history = NULL;
old_cmd_t* _history_tail = NULL;


#define ENV_PATH "PATH"

static int write_all_retry(int fd, const void* buf, size_t len) {
	const char* p = (const char*)buf;
	size_t off = 0;
	while(off < len) {
		ssize_t wr = write(fd, p + off, len - off);
		if(wr > 0) {
			off += (size_t)wr;
			continue;
		}
		if(errno == EAGAIN || errno == EINTR) {
			proc_usleep(1000);
			continue;
		}
		if(wr == 0 && errno == 0)
			errno = EPIPE;
		return -1;
	}
	return 0;
}

static int32_t find_exec(char* cmd, char* fname, char* full_cmd) {
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
		if(vfs_get_by_name(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
			cmd[at] = c;
			strcpy(full_cmd, cmd);
			return 0;
		}
	}
	else if(cmd[0] == '.' && cmd[1] == '/') { //current path
		char cwd[FS_FULL_NAME_MAX+1];
		const char* path = getcwd(cwd, FS_FULL_NAME_MAX);
		snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd+2);
		if(vfs_get_by_name(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
			cmd[at] = c;
			snprintf(full_cmd, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd+2);
			return 0;
		}
	}
	//search executable file in PATH dirs.
	const char* paths = getenv(ENV_PATH);
	if(paths == NULL)
		paths = "";
	char path[FS_FULL_NAME_MAX] = {0};
	i = 0;
	while(1) {
		if(paths[i] == 0 || paths[i] == ':') {
			strncpy(path, paths, i);
			path[i] = 0;
			if(path[0] != 0) {
				snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
				if(vfs_get_by_name(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
					cmd[at] = c;
					snprintf(full_cmd, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
					return 0;
				}
			}
			if(paths[i] == 0)
				break;
			paths = paths+i+1;
			i = 0;
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

	char full_name[FS_FULL_NAME_MAX] = { 0 };
	char cwd[FS_FULL_NAME_MAX] = { 0 };
	if(fname[0] != '/')
		snprintf(full_name, FS_FULL_NAME_MAX-1, "%s/%s", 
				getcwd(cwd, FS_FULL_NAME_MAX-1),
				fname);
	else
		strncpy(full_name, fname, FS_FULL_NAME_MAX-1);

	if(in != 0) {
		int32_t fd = open(full_name, O_RDONLY);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 0);
		close(fd);
	}
	else {

		int32_t fd = open(full_name, O_WRONLY | O_CREAT | O_TRUNC);
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
	char full_cmd[FS_FULL_NAME_MAX];
	if(find_exec(cmd, fname, full_cmd) != 0) {
		printf("'%s' not found!\n", cmd);
		return -1;
	}

	if(access(fname, X_OK) != 0) {
		printf("'%s' inexecutable!\n", fname);
		return -1;
	}

	proc_exec(full_cmd);
	return 0;
}

static void close_exec_fds(void) {
	for(int fd = 3; fd < MAX_OPEN_FILE_PER_PROC; fd++) {
		if(fd == VFS_BACKUP_FD0)
			continue;
		close(fd);
	}
}

static char* find_unquoted_pipe(char* cmd) {
	bool inQ = false;
	while(*cmd != 0) {
		if(*cmd == '"')
			inQ = !inQ;
		else if(!inQ && *cmd == '|')
			return cmd;
		cmd++;
	}
	return NULL;
}

static int do_stage_cmd(char* cmd) {
	char* proc = NULL;
	bool inQ = false;
	while(*cmd != 0) {
		char c = *cmd++;
		if(c == '"')
			inQ = !inQ;

		if(!inQ && proc == NULL && c == ' ')
			continue;

		if(!inQ && c == '>') {
			*(cmd-1) = 0;
			redir(cmd, 0);
			return do_cmd(proc);
		}
		else if(!inQ && c == '<') {
			*(cmd-1) = 0;
			redir(cmd, 1);
			return do_cmd(proc);
		}
		else if(proc == NULL) {
			proc = cmd-1;
		}
	}

	if(proc != NULL)
		return do_cmd(proc);
	return 0;
}

static int run_cmd(char* cmd);
static int do_pipe_cmd(char* cmd) {
	char* stages[32];
	int pids[32];
	int stage_num = 0;
	int prev_read = -1;
	char* stage = cmd;

	while(stage != NULL && stage_num < 32) {
		char* sep = find_unquoted_pipe(stage);
		if(sep != NULL) {
			*sep = 0;
			stages[stage_num++] = stage;
			stage = sep + 1;
			continue;
		}
		stages[stage_num++] = stage;
		break;
	}

	if(stage_num <= 1)
		return do_stage_cmd(cmd);

	for(int i = 0; i < stage_num; i++) {
		int fds[2] = {-1, -1};
		bool last = (i == stage_num - 1);
		if(!last && pipe(fds) != 0) {
			if(prev_read >= 0)
				close(prev_read);
			printf("pipe create failed!\n");
			return -1;
		}

		int pid = fork();
		if(pid < 0) {
			if(prev_read >= 0)
				close(prev_read);
			if(fds[0] >= 0)
				close(fds[0]);
			if(fds[1] >= 0)
				close(fds[1]);
			printf("fork failed! errno=%d\n", errno);
			return -1;
		}

		if(pid == 0) {
			if(prev_read >= 0) {
				dup2(prev_read, 0);
			}
			if(!last) {
				dup2(fds[1], 1);
			}
			if(prev_read >= 0)
				close(prev_read);
			if(fds[0] >= 0)
				close(fds[0]);
			if(fds[1] >= 0)
				close(fds[1]);
			close_exec_fds();
			exit(do_stage_cmd(stages[i]));
		}

		pids[i] = pid;
		if(prev_read >= 0)
			close(prev_read);
		if(!last) {
			close(fds[1]);
			prev_read = fds[0];
		}
		else {
			prev_read = -1;
		}
	}

	if(prev_read >= 0)
		close(prev_read);

	for(int i = 0; i < stage_num; i++) {
		ewok_waitpid(pids[i]);
	}
	return 0;
}

static int run_cmd(char* cmd) {
	if(find_unquoted_pipe(cmd) != NULL)
		return do_pipe_cmd(cmd);
	return do_stage_cmd(cmd);
}

static void prompt(void) {
	int uid = getuid();
	const char* cid = getenv("CONSOLE_ID");
	const char* user = getenv("USER");
	if(cid == NULL || cid[0] == 0)
		cid = "unknown";
	char cwd[FS_FULL_NAME_MAX+1];
	char out[FS_FULL_NAME_MAX+64];
	const char* mark = (uid == 0) ? "#" : "$";
	const char* path = getcwd(cwd, FS_FULL_NAME_MAX);
	int len;
	if(path == NULL)
		path = "/";

	if(user != NULL && user[0] != 0)
		len = snprintf(out, sizeof(out), "\033[4m[%s:%s]:%s%s\033[0m ", cid, user, path, mark);
	else
		len = snprintf(out, sizeof(out), "\033[4m[%s]:%s%s\033[0m ", cid, path, mark);
	if(len > 0) {
		if((size_t)len >= sizeof(out))
			len = (int)sizeof(out) - 1;
		write_all_retry(1, out, (size_t)len);
	}
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "i");
		if(c == -1)
			break;

		switch (c) {
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char* argv[]) {
	_script_mode = false;
	_history = NULL;
	_terminated = 0;
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	int argind = doargs(argc, argv);

	int fd_in = 0;
	if(argind < argc) {
		fd_in = open(argv[argind], O_RDONLY);
		if(fd_in < 0)
			return -1;
		_script_mode = true;
	}

	setenv("PATH", "/sbin:/bin:/bin/x");

	const char* home = getenv("HOME");
	if(home == NULL || home[0] == 0)
		home = "/";
	chdir(home);
	str_t* cmdstr = str_new("");
	while(_terminated == 0) {
		if(fd_in == 0) {
			prompt();
		}

		int gets_res = cmd_gets(fd_in, cmdstr);
		if(gets_res != 0 && cmdstr->len == 0)
			break;

		char* cmd = cmdstr->cstr;
		if(cmd[0] == 0)
			continue;

		if(cmd[0] == '#')
			continue;
		if(cmd[0] == '@')
			cmd++;
		else if(_script_mode)
			printf("%s\n", cmd);

		add_history(cmdstr->cstr);
		if(handle_shell_cmd(cmd) == 0)
			continue;

		int len = strlen(cmd)-1;
		int fg = 1;
		if(cmd[len] == '&') {
			cmd[len] = 0;
			fg = 0;
		}	

		int child_pid = fork();
		if (child_pid == 0) {
			if(fg == 0 || _script_mode)
				proc_detach();
			close_exec_fds();
			int res = run_cmd(cmd);
			str_free(cmdstr);	
			return res;
		}
		else if(child_pid < 0) {
			printf("fork failed! errno=%d\n", errno);
		}
		else if(fg != 0) {
			ewok_waitpid(child_pid);
		}
	}
	if(fd_in > 0) //close initrd file
		close(fd_in);
	free_history();
	return 0;
}
