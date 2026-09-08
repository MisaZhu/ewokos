#ifndef EWOKOS_LIBC_UNISTD_H
#define EWOKOS_LIBC_UNISTD_H

#include <stddef.h>
#include <sys/types.h>

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

/* sysconf() name values */
#define _SC_ARG_MAX            0
#define _SC_CHILD_MAX          1
#define _SC_CLK_TCK            2
#define _SC_NGROUPS_MAX        3
#define _SC_OPEN_MAX           4
#define _SC_STREAM_MAX         5
#define _SC_TZNAME_MAX         6
#define _SC_JOB_CONTROL        7
#define _SC_SAVED_IDS          8
#define _SC_VERSION            9
#define _SC_PAGESIZE           10
#define _SC_PAGE_SIZE          _SC_PAGESIZE
#define _SC_NPROCESSORS_CONF   11
#define _SC_NPROCESSORS_ONLN   12
#define _SC_PHYS_PAGES         13
#define _SC_AVPHYS_PAGES       14
#define _SC_THREAD_SAFE_FUNCTIONS 15
#define _SC_THREADS            16
#define _SC_REENTRANT          17
#define _SC_GETPW_R_SIZE_MAX   18
#define _SC_GETGR_R_SIZE_MAX   19
#define _SC_LOGIN_NAME_MAX     20
#define _SC_TTY_NAME_MAX       21

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define L_ctermid 16

#ifdef __cplusplus
extern "C" {
#endif

int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
int ioctl(int fd, int cmd, ...);

int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
int fork(void);
int isatty(int fd);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execv(const char *pathname, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execl(const char *pathname, const char *arg, ...);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int chown(const char *pathname, int uid, int gid);
int unlink(const char *pathname);
int rmdir(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int access(const char *pathname, int mode);
int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

uid_t getuid(void);
gid_t getgid(void);
uid_t geteuid(void);
gid_t getegid(void);
pid_t getpid(void);
pid_t getppid(void);
pid_t gettid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);

pid_t getpgid(pid_t pid);
pid_t getpgrp(void);
int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
pid_t getsid(pid_t pid);

int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);
int fsync(int fd);
int fdatasync(int fd);
void sync(void);
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

int daemon(int nochdir, int noclose);
int gethostname(char *name, size_t len);
int sethostname(const char *name, size_t len);
char *ttyname(int fd);
int ttyname_r(int fd, char *buf, size_t buflen);
char *ctermid(char *s);
long sysconf(int name);
int nice(int inc);
int getpagesize(void);

unsigned int alarm(unsigned int seconds);
int pause(void);

void _exit(int status);

ssize_t readlink(const char *path, char *buf, size_t bufsiz);
char *get_current_dir_name(void);

#ifdef __cplusplus
}
#endif

#endif
