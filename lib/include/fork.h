#ifndef FORK_H
#define FORK_H

int fork();
int getpid();
int exec(const char* cmd);
int execElf(const char* img);
void exit(int code);
void wait(int pid);
void yield();

#endif
