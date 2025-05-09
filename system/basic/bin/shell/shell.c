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
#include <ewoksys/klog.h>
#include <setenv.h>
#include "shell.h"

bool _script_mode = false;
bool _terminated = false;

old_cmd_t* _history = NULL;
old_cmd_t* _history_tail = NULL;


#define ENV_PATH "PATH"

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

static int run_cmd(char* cmd);
static int do_pipe_cmd(char* p1, char* p2) {
	int fds[2];
	if(pipe(fds) != 0) {
		printf("pipe create failed!\n");
		return -1;
	}

	int pid = syscall0(SYS_FORK);
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
	bool inQ = false;
	while(*cmd != 0) {
		char c = *cmd++;
		if(c == '"') 
			inQ = !inQ;
		
		if(!inQ && proc == NULL && c == ' ')
			continue;

		if(!inQ && c == '>') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 0); //redir OUT.
			return do_cmd(proc);
		}
		else if(!inQ && c == '<') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 1); //redir in.
			return do_cmd(proc);
		}
		else if(!inQ && c == '|') { //pipe
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

static void prompt(void) {
	int uid = getuid();
	const char* cid = getenv("CONSOLE_ID");
	if(cid == NULL || cid[0] == 0)
		cid = "unknown";
	char cwd[FS_FULL_NAME_MAX+1];
	if(uid == 0)
		printf("\033[4m[%s]:%s#\033[0m ", cid, getcwd(cwd, FS_FULL_NAME_MAX));
	else
		printf("\033[4m[%s]:%s$\033[0m ", cid, getcwd(cwd, FS_FULL_NAME_MAX));
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
		if(fd_in == 0)
			prompt();

		if(cmd_gets(fd_in, cmdstr) != 0 && cmdstr->len == 0)
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
			int res = run_cmd(cmd);
			str_free(cmdstr);	
			return res;
		}
		else if(fg != 0) {
			waitpid(child_pid);
		}
	}
	if(fd_in > 0) //close initrd file
		close(fd_in);
	free_history();
	return 0;
}
