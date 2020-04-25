#ifndef KSERV_H
#define KSERV_H

enum {
	KSERV_VFS = 0,
	KSERV_MAX
};

enum {
	KSERV_CMD_REG = 0,
	KSERV_CMD_UNREG,
	KSERV_CMD_GET
};

int kserv_reg(int kserv_id);
int kserv_unreg(int kserv_id);
int kserv_get(int kserv_id);

#endif
