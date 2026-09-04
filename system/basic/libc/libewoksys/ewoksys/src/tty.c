#include <ewoksys/tty.h>
#include <string.h>
#include <signal.h>

/*
 * Shared terminal line discipline.
 *
 * The default attributes are RAW, so tty_input/tty_output are pass-through
 * until an application enables ICANON/ECHO/ISIG/OPOST/... via tcsetattr. That
 * keeps the historically always-raw EwokOS terminals (whose shells do their own
 * echo and line editing) behaving exactly as before.
 */

static inline void tty_emit(tty_emit_fn fn, void* arg, char c) {
	if(fn != NULL)
		fn(arg, &c, 1);
}

void tty_init(tty_state_t* st) {
	if(st == NULL)
		return;
	memset(st, 0, sizeof(*st));
	st->tio.c_iflag = 0;
	st->tio.c_oflag = 0;
	st->tio.c_cflag = CS8 | CREAD | CLOCAL;
	st->tio.c_lflag = 0;              /* raw: no ICANON/ECHO/ISIG */
	st->tio.c_cc[VEOF] = 4;           /* ^D */
	st->tio.c_cc[VEOL] = '\n';
	st->tio.c_cc[VERASE] = 0x7f;
	st->tio.c_cc[VINTR] = 3;          /* ^C */
	st->tio.c_cc[VQUIT] = 0x1c;       /* ^\ */
	st->tio.c_cc[VKILL] = 0x15;       /* ^U */
	st->tio.c_cc[VSTART] = 0x11;      /* ^Q */
	st->tio.c_cc[VSTOP] = 0x13;       /* ^S */
	st->tio.c_cc[VSUSP] = 0x1a;       /* ^Z */
	st->tio.c_cc[VMIN] = 1;
	st->tio.c_cc[VTIME] = 0;
	st->tio.c_ispeed = B115200;
	st->tio.c_ospeed = B115200;
	st->line_len = 0;
	st->fg_pid = -1;
	st->stopped = false;
}

void tty_set_foreground(tty_state_t* st, int pid) {
	if(st != NULL)
		st->fg_pid = pid;
}

void tty_set_winsize(tty_state_t* st, unsigned short rows, unsigned short cols) {
	if(st == NULL)
		return;
	st->ws.ws_row = rows;
	st->ws.ws_col = cols;
}

/*
 * EwokOS exposes only SYS_SIG_STOP and SYS_SIG_KILL (no catchable SIGINT) and
 * tracks no foreground process group, so ISIG maps VINTR/VQUIT to KILL and
 * VSUSP to STOP, delivered to the process that read most recently.
 */
static void tty_signal(tty_state_t* st, char c) {
	if(st->fg_pid <= 0)
		return;
	if(c == (char)st->tio.c_cc[VSUSP])
		kill(st->fg_pid, SYS_SIG_STOP);
	else
		kill(st->fg_pid, SYS_SIG_KILL);
}

/* Echo one byte honoring ECHO/ECHONL. Control chars are only echoed as part of
 * the higher-level ECHOE/ECHOK handling below. */
static void tty_echo_byte(tty_state_t* st, char c, tty_emit_fn emit_echo, void* echo_arg) {
	if(emit_echo == NULL)
		return;
	if(c == '\n' || c == '\r') {
		if((st->tio.c_lflag & (ECHO | ECHONL)) != 0)
			tty_emit(emit_echo, echo_arg, c);
		return;
	}
	if((st->tio.c_lflag & ECHO) != 0)
		tty_emit(emit_echo, echo_arg, c);
}

/* Deliver the pending canonical line to the reader and reset it. */
static void tty_line_flush(tty_state_t* st, tty_emit_fn emit_read, void* read_arg) {
	if(emit_read != NULL && st->line_len > 0)
		emit_read(read_arg, st->line, (int)st->line_len);
	st->line_len = 0;
}

static inline void tty_line_push(tty_state_t* st, char c) {
	if(st->line_len < TTY_LINE_MAX - 1)
		st->line[st->line_len++] = c;
}

void tty_input(tty_state_t* st, const char* in, int n,
		tty_emit_fn emit_read, void* read_arg,
		tty_emit_fn emit_echo, void* echo_arg) {
	if(st == NULL || in == NULL || n <= 0)
		return;

	struct termios* t = &st->tio;
	bool canon = (t->c_lflag & ICANON) != 0;

	for(int i = 0; i < n; i++) {
		unsigned char uc = (unsigned char)in[i];

		if((t->c_iflag & ISTRIP) != 0)
			uc &= 0x7f;

		if(uc == '\r') {
			if((t->c_iflag & IGNCR) != 0)
				continue;
			if((t->c_iflag & ICRNL) != 0)
				uc = '\n';
		}
		else if(uc == '\n') {
			if((t->c_iflag & INLCR) != 0)
				uc = '\r';
		}

		char c = (char)uc;

		/* ISIG: signal chars are consumed, never delivered to the app. */
		if((t->c_lflag & ISIG) != 0) {
			if(c == (char)t->c_cc[VINTR] || c == (char)t->c_cc[VQUIT]) {
				tty_signal(st, c);
				if((t->c_lflag & NOFLSH) == 0)
					st->line_len = 0;
				continue;
			}
			if(c == (char)t->c_cc[VSUSP]) {
				tty_signal(st, c);
				continue;
			}
		}

		/* IXON: VSTOP/VSTART toggle output flow and are not delivered. */
		if((t->c_iflag & IXON) != 0) {
			if(c == (char)t->c_cc[VSTOP]) { st->stopped = true; continue; }
			if(c == (char)t->c_cc[VSTART]) { st->stopped = false; continue; }
		}

		if(canon) {
			if(c == (char)t->c_cc[VERASE]) {
				if(st->line_len > 0) {
					st->line_len--;
					if((t->c_lflag & ECHOE) != 0) {
						tty_emit(emit_echo, echo_arg, '\b');
						tty_emit(emit_echo, echo_arg, ' ');
						tty_emit(emit_echo, echo_arg, '\b');
					}
				}
				continue;
			}
			if(c == (char)t->c_cc[VKILL]) {
				st->line_len = 0;
				if((t->c_lflag & ECHOK) != 0)
					tty_emit(emit_echo, echo_arg, '\n');
				continue;
			}
			if(c == (char)t->c_cc[VEOF]) {
				/* Deliver the partial line; EOF itself is not stored. */
				tty_line_flush(st, emit_read, read_arg);
				continue;
			}
			if(c == '\n' || c == (char)t->c_cc[VEOL]) {
				tty_echo_byte(st, c, emit_echo, echo_arg);
				tty_line_push(st, c);
				tty_line_flush(st, emit_read, read_arg);
				continue;
			}
			tty_echo_byte(st, c, emit_echo, echo_arg);
			tty_line_push(st, c);
			continue;
		}

		/* Non-canonical: deliver each byte immediately (VMIN>=1,VTIME=0). */
		tty_echo_byte(st, c, emit_echo, echo_arg);
		tty_emit(emit_read, read_arg, c);
	}
}

int tty_output(tty_state_t* st, const char* in, int n, char* out, int outmax) {
	if(st == NULL || in == NULL || out == NULL || n <= 0 || outmax <= 0)
		return 0;

	struct termios* t = &st->tio;
	if((t->c_oflag & OPOST) == 0) {
		int m = (n < outmax) ? n : outmax;
		memcpy(out, in, (size_t)m);
		return m;
	}

	int o = 0;
	for(int i = 0; i < n && o < outmax; i++) {
		char c = in[i];
		if(c == '\n' && (t->c_oflag & ONLCR) != 0) {
			if(o + 2 > outmax)
				break;
			out[o++] = '\r';
			out[o++] = '\n';
		}
		else if(c == '\r' && (t->c_oflag & OCRNL) != 0) {
			out[o++] = '\n';
		}
		else {
			out[o++] = c;
		}
	}
	return o;
}

int tty_dev_cntl(tty_state_t* st, int cmd, proto_t* in, proto_t* ret) {
	if(st == NULL)
		return -1;

	switch(cmd) {
	case TCGETS:
		PF->add(ret, &st->tio, sizeof(struct termios));
		return 0;
	case TCSETS:
	case TCSETSW:
	case TCSETSF: {
		int32_t sz = 0;
		void* data = proto_read(in, &sz);
		if(data == NULL || sz != (int32_t)sizeof(struct termios))
			return -1;
		if(cmd == TCSETSF)
			st->line_len = 0;   /* flush pending canonical input */
		memcpy(&st->tio, data, sizeof(struct termios));
		return 0;
	}
	case TIOCGWINSZ:
		PF->add(ret, &st->ws, sizeof(struct winsize));
		return 0;
	case TIOCSWINSZ: {
		int32_t sz = 0;
		void* data = proto_read(in, &sz);
		if(data == NULL || sz != (int32_t)sizeof(struct winsize))
			return -1;
		memcpy(&st->ws, data, sizeof(struct winsize));
		return 0;
	}
	default:
		return -1;
	}
}
