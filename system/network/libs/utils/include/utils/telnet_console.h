#ifndef TELNET_CONSOLE_H
#define TELNET_CONSOLE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/vfs.h>

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251
#define TELNET_SB   250
#define TELNET_SE   240

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

typedef struct {
	int raw_buf_off;
	int raw_buf_len;
	uint8_t raw_buf[256];
	int state;
	int pending_cr_tail;
	uint8_t verb;
} telnet_console_t;

enum {
	TELNET_STATE_DATA = 0,
	TELNET_STATE_IAC,
	TELNET_STATE_CMD,
	TELNET_STATE_SB,
	TELNET_STATE_SB_DATA,
	TELNET_STATE_SB_IAC,
};

static inline void telnet_console_init(telnet_console_t* tc) {
	if(tc == NULL)
		return;
	memset(tc, 0, sizeof(*tc));
	tc->state = TELNET_STATE_DATA;
}

static inline int telnet_console_is_active(void) {
	const char* cid = getenv("CONSOLE_ID");
	return cid != NULL && strcmp(cid, "telnet") == 0;
}

static inline void telnet_console_reply_option(uint8_t verb, uint8_t opt) {
	uint8_t reply[3] = { TELNET_IAC, 0, opt };
	int len = 3;
	int supported = (opt == TELNET_OPT_ECHO || opt == TELNET_OPT_SUPPRESS_GA);
	int out_fd = VFS_BACKUP_FD1;

	if(out_fd < 0)
		out_fd = VFS_BACKUP_FD0;
	if(out_fd < 0)
		out_fd = 1;

	switch(verb) {
	case TELNET_DO:
		if(!supported)
			reply[1] = TELNET_WONT;
		else
			len = 0;
		break;
	case TELNET_DONT:
		len = 0;
		break;
	case TELNET_WILL:
		if(!supported)
			reply[1] = TELNET_DONT;
		else
			len = 0;
		break;
	case TELNET_WONT:
		len = 0;
		break;
	default:
		len = 0;
		break;
	}

	if(len > 0) {
		/*
		 * Telnet option replies are protocol bytes, not user-facing output.
		 * They must bypass stdout/stderr relays and go straight to the raw socket.
		 */
		(void)write(out_fd, reply, len);
	}
}

static inline uint8_t telnet_console_parse(telnet_console_t* tc, uint8_t c) {
	uint8_t ret = 0;

	if(tc == NULL)
		return c;

	if(tc->state == TELNET_STATE_DATA && tc->pending_cr_tail != 0) {
		tc->pending_cr_tail = 0;
		if(c == 0 || c == '\n')
			return 0;
	}

	switch(tc->state) {
	case TELNET_STATE_DATA:
		if(c == TELNET_IAC)
			tc->state = TELNET_STATE_IAC;
		else if(c == '\r') {
			tc->pending_cr_tail = 1;
			ret = '\n';
		}
		else
			ret = c;
		break;
	case TELNET_STATE_IAC:
		if(c == TELNET_IAC) {
			ret = TELNET_IAC;
			tc->state = TELNET_STATE_DATA;
		}
		else if(c == TELNET_SB)
			tc->state = TELNET_STATE_SB;
		else if(c == TELNET_WILL || c == TELNET_WONT ||
				c == TELNET_DO || c == TELNET_DONT) {
			tc->verb = c;
			tc->state = TELNET_STATE_CMD;
		}
		else if(c == TELNET_SE)
			tc->state = TELNET_STATE_DATA;
		else
			tc->state = TELNET_STATE_DATA;
		break;
	case TELNET_STATE_CMD:
		telnet_console_reply_option(tc->verb, c);
		tc->state = TELNET_STATE_DATA;
		break;
	case TELNET_STATE_SB:
		tc->state = TELNET_STATE_SB_DATA;
		break;
	case TELNET_STATE_SB_DATA:
		if(c == TELNET_IAC)
			tc->state = TELNET_STATE_SB_IAC;
		break;
	case TELNET_STATE_SB_IAC:
		if(c == TELNET_SE)
			tc->state = TELNET_STATE_DATA;
		else if(c != TELNET_IAC)
			tc->state = TELNET_STATE_SB_DATA;
		break;
	default:
		tc->state = TELNET_STATE_DATA;
		break;
	}

	return ret;
}

static inline int telnet_console_read(int fd, telnet_console_t* tc, char* c) {
	if(tc == NULL || c == NULL)
		return -1;

	if(tc->raw_buf_off < tc->raw_buf_len) {
		*c = (char)tc->raw_buf[tc->raw_buf_off++];
		return 1;
	}

	int ret = read(fd, tc->raw_buf, sizeof(tc->raw_buf));
	if(ret <= 0)
		return ret;

	tc->raw_buf_off = 1;
	tc->raw_buf_len = ret;
	*c = (char)tc->raw_buf[0];
	return 1;
}

#endif
