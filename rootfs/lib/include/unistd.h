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

void yield();

int getuid();

unsigned int sleep(unsigned int secs);
unsigned int usleep(unsigned int msecs);

/*i/o functions*/
#define O_RDONLY 0

int open(const char* fname, int mode);

int write(int fd, const void* buf, uint32_t mode);

int read(int fd, void* buf, uint32_t mode);

void close(int fd);

#endif
