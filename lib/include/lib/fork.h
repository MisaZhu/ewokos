#ifndef FORK_H
#define FORK_H

int fork();
int exec(const char* cmd);
void exit(int code);
void wait(int pid);
void yield();

#endif
