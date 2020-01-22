#ifndef UNISTD_H
#define UNISTD_H

#include <stdint.h>
#include <errno.h>
#include <sys/cmain.h>

#define ERR_RETRY -2

int getpid(void);
int fork(void);
void detach(void);
unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usecs);

int read(int fd, void* buf, uint32_t size);
int read_nblock(int fd, void* buf, uint32_t size);
int write(int fd, const void* buf, uint32_t size);
int write_nblock(int fd, const void* buf, uint32_t size);

int read_block(int pid, void* buf, uint32_t size, int32_t index);
int write_block(int pid, const void* buf, uint32_t size, int32_t index);

int unlink(const char* fname);

int lseek(int fd, uint32_t offset, int whence);

void exec_elf(const char* cmd_line, const char* elf, int32_t size);
int exec(const char* cmd_line);

char* getcwd(char* buf, uint32_t size);
int chdir(const char* path);

int dup2(int from, int to);
int dup(int from);

int pipe(int fds[2]);

#endif
