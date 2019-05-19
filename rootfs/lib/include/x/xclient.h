#ifndef XCLIENT_H
#define XCLIENT_H

#include <proto.h>
#include <vfs/fs.h>

enum {
	X_CMD_NONE = FS_CTRL_CUSTMIZE,
	X_CMD_MOVE_TO,
	X_CMD_RESIZE_TO,
	X_CMD_CLEAR,
	X_CMD_LINE,
	X_CMD_BOX,
	X_CMD_FILL,
	X_CMD_STR,
	X_CMD_SET_FG,
	X_CMD_SET_BG,
	X_CMD_SET_FONT
};

int32_t x_open(const char* dev);
void x_close(int32_t fd);
int32_t x_cmd(int32_t fd, int32_t cmd, proto_t* in, proto_t* out);


#endif
