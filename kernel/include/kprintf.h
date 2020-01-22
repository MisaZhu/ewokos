#ifndef PRINTK_H
#define PRINTK_H

#include <console.h>

void init_console(void);
void setup_console(void);
void close_console(void);

void uart_out(const char* s);
void printf(const char *format, ...);

void flush_actled(void);

#endif
