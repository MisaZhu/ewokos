#ifndef USERMAN_H
#define USERMAN_H

#include <types.h>

#define KSERV_USERMAN_NAME  "USER_MAN"

#define LOGIN_DATA_LEN  128

typedef enum {
	USERMAN_AUTH = 0,
} UserManCmdT;

int usermanAuth(const char* name, const char* passwd);

#endif
