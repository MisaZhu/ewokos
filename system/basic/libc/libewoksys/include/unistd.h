#ifndef EWOKOS_LIBC_UNISTD_H
#define EWOKOS_LIBC_UNISTD_H

#include <stddef.h>
#include <stdint.h>

typedef int uid_t;
typedef int gid_t;
typedef long ssize_t;
typedef long off_t;

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

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
int pipe(int pipefd[2]);
int fork(void);

int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int chown(const char *pathname, int uid, int gid);
int access(const char *pathname, int mode);
int getopt(int argc, char * const argv[], const char *optstring);

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

unsigned int sleep(unsigned int seconds);
int usleep(uint32_t usec);

uid_t getuid(void);
gid_t getgid(void);
int getpid(void);
int gettid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);

#ifdef __cplusplus
}
#endif

#endif
