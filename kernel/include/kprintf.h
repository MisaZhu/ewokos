#ifndef PRINTK_H
#define PRINTK_H

void uart_out(const char* s);
void printf(const char *format, ...);
void flush_actled(void);

#endif
