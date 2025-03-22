#ifndef PRINTK_H
#define PRINTK_H

void kout(const char* s);
void printf(const char *format, ...);
void kprintf(const char *format, ...);
void kprintf_init(void);
void kprintf_close(void);

#endif
