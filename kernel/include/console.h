#ifndef CONSOLE_H
#define CONSOLE_H

#include <types.h>

/* function declarations for interfacing with concole */
void consoleInit(void);
void kputch(int c);
void kputs(const char* str);

#endif
