#ifndef UNISTD_H
#define UNISTD_H

#include <types.h>
#include <cmain.h>

int chdir(const char* dir);

char* getcwd(char* buf, uint32_t size);

int fork();

int getpid();

int exec(const char* cmd);

void exit(int code);

void wait(int pid);

int getuid();

unsigned int sleep(unsigned int secs);
unsigned int usleep(unsigned int msecs);

/*i/o functions*/
#define O_RDONLY 0
#define O_WRONLY 2
#define O_RDWR	 3

#define O_CREAT	 8

int open(const char* fname, int flags);

int write(int fd, const void* buf, uint32_t size);

int read(int fd, void* buf, uint32_t size);

void close(int fd);

int pipe(int fds[2]);

#endif
