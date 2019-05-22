#ifndef XCMD_H
#define XCMD_H

#include <proto.h>
#include <vfs/fs.h>

enum {
	X_CMD_NONE = FS_CTRL_CUSTMIZE,
	X_CMD_REG_WM,
	X_CMD_FLUSH,
	X_CMD_MOVE_TO,
	X_CMD_RESIZE_TO,
	X_CMD_SET_TITLE,
	X_CMD_SET_STYLE,
	X_CMD_SET_STATE,
	X_CMD_GET_SCR_SIZE,
	X_CMD_GET_EVENT
};

int32_t x_cmd(int32_t fd, int32_t cmd, proto_t* in, proto_t* out);


#endif
