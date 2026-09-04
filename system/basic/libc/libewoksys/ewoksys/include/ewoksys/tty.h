#ifndef EWOKSYS_TTY_H
#define EWOKSYS_TTY_H

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Longest line buffered in canonical (ICANON) mode before it is delivered. */
#define TTY_LINE_MAX 1024

/*
 * Emit callback: delivers processed bytes either to the application's read()
 * (emit_read) or back to the terminal output for echoing (emit_echo).
 */
typedef void (*tty_emit_fn)(void* arg, const char* buf, int size);

/*
 * Per-device terminal state shared by every tty driver (uartd/ttyd/consoled/
 * xterm/pts). Attributes default to RAW so the historically always-raw EwokOS
 * terminals keep behaving identically until an application enables a flag.
 */
typedef struct {
	struct termios tio;             /* current terminal attributes */
	struct winsize ws;              /* window size; 0,0 = unknown */
	char     line[TTY_LINE_MAX];    /* canonical line-editing buffer */
	uint32_t line_len;
	int      fg_pid;                /* last reader; ISIG signal target */
	bool     stopped;               /* IXON: VSTOP seen, VSTART not yet */
} tty_state_t;

/* Initialise to the raw default and reset all line/flow state. */
void tty_init(tty_state_t* st);

/* Process input bytes arriving from the underlying device. Applies the input
 * line discipline (ICRNL/INLCR/IGNCR/ISTRIP, ISIG, IXON, ICANON line editing,
 * ECHO*) and emits application-readable bytes via emit_read and echoed bytes
 * via emit_echo. Either callback may be NULL. */
void tty_input(tty_state_t* st, const char* in, int n,
		tty_emit_fn emit_read, void* read_arg,
		tty_emit_fn emit_echo, void* echo_arg);

/* Apply the output line discipline (OPOST/ONLCR/OCRNL) to n bytes from in,
 * writing at most outmax bytes to out. Returns the number of bytes written.
 * With OPOST off (the default) this is a plain copy. */
int tty_output(tty_state_t* st, const char* in, int n, char* out, int outmax);

/* Handle a terminal dev_cntl request. Returns 0 on success, negative when cmd
 * is not one of TCGETS/TCSETS/TCSETSW/TCSETSF/TIOCGWINSZ/TIOCSWINSZ. */
int tty_dev_cntl(tty_state_t* st, int cmd, proto_t* in, proto_t* ret);

/* Record the process that ISIG control characters should signal. */
void tty_set_foreground(tty_state_t* st, int pid);

/* Drivers that know their geometry (GUI consoles) report it so TIOCGWINSZ can
 * answer without a probe. rows/cols of 0 mean "unknown". */
void tty_set_winsize(tty_state_t* st, unsigned short rows, unsigned short cols);

#ifdef __cplusplus
}
#endif

#endif
