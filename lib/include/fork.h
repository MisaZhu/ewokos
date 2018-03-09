#ifndef FORK_H
#define FORK_H

int fork();
int getpid();
int exec(const char* cmd);
void exit(int code);
void wait(int pid);
void yield();

#endif
