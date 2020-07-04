#ifndef CRITICAL_H
#define CRITICAL_H

#ifdef __cplusplus
extern "C" {
#endif


void critical_enter(void);
void critical_quit(void);

#ifdef __cplusplus
}
#endif


#endif
