#ifndef KCONSOLE_H
#define KCONSOLE_H

#ifdef KCONSOLE
void kconsole_init(void);
void kconsole_input(const char* s);
void kconsole_close(void);
#endif

#endif