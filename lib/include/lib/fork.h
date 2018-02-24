#ifndef FORK_H
#define FORK_H

int fork();
void exit(int code);
void wait(int pid);
void yield();

#endif
