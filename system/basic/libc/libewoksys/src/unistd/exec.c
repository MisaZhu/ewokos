#include <unistd.h>

extern char **environ;
extern int _execve(const char *name, char *const argv[], char *const env[]);

int execve(const char *pathname, char *const argv[], char *const envp[]) {
	return _execve(pathname, argv, envp);
}

int execv(const char *pathname, char *const argv[]) {
	return _execve(pathname, argv, environ);
}
