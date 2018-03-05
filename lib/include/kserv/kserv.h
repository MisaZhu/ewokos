#ifndef KSERV_H
#define KSERV_H

typedef enum {
	KS_REG = 0,
	KS_GET,
} KSCmdT;

int getKServPid(const char* name);

#endif
