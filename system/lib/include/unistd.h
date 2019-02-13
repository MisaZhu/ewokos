#ifndef UNISTD_H
#define UNISTD_H

#include <types.h>

int chdir(const char* dir);

char* getcwd(char* buf, uint32_t size);

int fork();

int getpid();

int exec(const char* cmd);

void exit(int code);

void wait(int pid);

void yield();

int getuid();

#endif
