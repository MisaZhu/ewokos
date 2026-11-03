/*
 * more - browse or page through a text file.
 *
 * A from-scratch pager whose command-line options and interactive commands
 * follow util-linux more(1), so shell habits and scripts keep working:
 *
 *   more [-dflpcsu] [-n lines] [-Vh] [+line] [+/pattern] [file ...]
 *
 * Terminal geometry comes from TIOCGWINSZ (the GUI consoles publish it) with
 * the ANSI cursor-position-report probe as a fallback for the serial console.
 * Keys are read one at a time; EwokOS terminals default to a raw line
 * discipline, and we still put the tty into raw mode explicitly (and restore
 * it on the way out) so canonical callers behave too.
 *
 * When stdout is not a terminal, or no key source is available, more degrades
 * to a plain pass-through (like cat) and emits no escape sequences.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <getopt.h>
#include <termios.h>
#include <regex.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#define MORE_VERSION "more from util-linux 2.40 (EwokOS port)"

/* ---- terminal escape sequences ---------------------------------------- */
#define ESC               "\033"
#define ESC_HOME          ESC "[H"
#define ESC_CLEAR2EOL     ESC "[K"
#define ESC_CLEAR2EOS     ESC "[J"
#define ESC_CLEAR_SCREEN  ESC "[2J"
#define ESC_HIDE_CURSOR   ESC "[?25l"
#define ESC_SHOW_CURSOR   ESC "[?25h"
#define ESC_REVERSE       ESC "[7m"
#define ESC_NORMAL        ESC "[m"
#define ESC_BELL          "\a"

#define CTRL(c) ((int)((c) & 0x1f))

/* Sentinel key codes returned by read_key(); all > 0xff so they never collide
 * with a real input byte. A lone ESC is reported as 0x1b itself. */
enum {
	KEY_ABORT = -1, /* read error / EOF on the key fd */
	KEY_UP = 0x100,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_PGUP,
	KEY_PGDN,
	KEY_HOME,
	KEY_END,
	KEY_ESC_V /* ESC v : back one screen */
};

/* ---- command-line options --------------------------------------------- */
static bool o_silent = false;    /* -d : help prompt instead of bell        */
static bool o_logical = false;   /* -f : count logical, not screen, lines   */
static bool o_nopause = false;   /* -l : no pause after a form feed         */
static bool o_printover = false; /* -p : clear screen, do not scroll        */
static bool o_cleanprint = false;/* -c : clean-print, do not scroll         */
static bool o_squeeze = false;   /* -s : squeeze runs of blank lines        */
static bool o_plain = false;     /* -u : suppress underlining/bold          */
static int  o_lines = 0;         /* -n : lines per screenful (0 = auto)     */
static long o_start_line = -1;   /* +num  : 1-based first line to show      */
static char* o_start_pat = NULL; /* +/pat : first line matching pattern     */

/* ---- runtime terminal state ------------------------------------------- */
static int  term_rows = 24;
static int  term_cols = 80;
static int  key_fd = -1;         /* fd keys are read from                   */
static bool interactive = false; /* full-screen paging is possible          */
static struct termios saved_tio;
static bool tio_saved = false;

/* ---- the document being paged ----------------------------------------- */
typedef struct {
	char*   data;      /* whole file bytes, NUL terminated               */
	size_t  data_len;
	size_t* off;       /* off[i] : byte offset of line i                 */
	int*    len;       /* len[i] : printable length (terminator stripped)*/
	long    nlines;
	long    off_cap;
} doc_t;

static doc_t doc;

/* file list for multi-file browsing */
static char** files = NULL;
static int    nfiles = 0;
static int    cur_file = 0;

/* pager state carried across the command loop */
static char   pending_msg[256];
static char   search_pat[256];
static bool   have_search = false;
static long   search_mark = -1;   /* position where the last search started */
static long   pending_count = 0;
static bool   have_count = false;

/* =========================================================================
 *  Document loading
 * ====================================================================== */

static int grow_off(doc_t* d, long need) {
	if(need <= d->off_cap)
		return 0;
	long cap = d->off_cap ? d->off_cap : 64;
	while(cap < need)
		cap *= 2;
	size_t* no = (size_t*)realloc(d->off, (size_t)cap * sizeof(size_t));
	if(no == NULL)
		return -1;
	int* nl = (int*)realloc(d->len, (size_t)cap * sizeof(int));
	if(nl == NULL) {
		d->off = no;
		return -1;
	}
	d->off = no;
	d->len = nl;
	d->off_cap = cap;
	return 0;
}

static void add_line(doc_t* d, size_t start, int len) {
	if(grow_off(d, d->nlines + 1) != 0)
		return;
	d->off[d->nlines] = start;
	d->len[d->nlines] = len;
	d->nlines++;
}

static bool line_is_blank(const char* s, int len) {
	for(int i = 0; i < len; i++) {
		if(!isspace((unsigned char)s[i]))
			return false;
	}
	return true;
}

/* Collapse runs of blank lines down to a single blank line (more -s). */
static void squeeze_blanks(doc_t* d) {
	long w = 0;
	bool prev_blank = false;
	for(long r = 0; r < d->nlines; r++) {
		const char* s = d->data + d->off[r];
		bool blank = line_is_blank(s, d->len[r]);
		if(blank && prev_blank)
			continue;
		d->off[w] = d->off[r];
		d->len[w] = d->len[r];
		w++;
		prev_blank = blank;
	}
	d->nlines = w;
}

/* Split the loaded bytes into a line index. A trailing newline does not
 * create an extra empty line, matching wc/more line counts. */
static void index_lines(doc_t* d) {
	size_t i = 0;
	while(i < d->data_len) {
		size_t start = i;
		while(i < d->data_len && d->data[i] != '\n')
			i++;
		size_t end = i; /* exclusive, points at '\n' or EOF */
		if(i < d->data_len)
			i++;    /* skip the '\n' */
		int len = (int)(end - start);
		while(len > 0 && (d->data[start + len - 1] == '\r' ||
		                  d->data[start + len - 1] == '\n'))
			len--;
		add_line(d, start, len);
	}
	if(o_squeeze)
		squeeze_blanks(d);
}

static char* read_all_fd(int fd, size_t* out_len) {
	size_t cap = 8192, len = 0;
	char* buf = (char*)malloc(cap);
	if(buf == NULL)
		return NULL;
	for(;;) {
		if(len + 4096 + 1 > cap) {
			size_t ncap = cap * 2;
			char* nb = (char*)realloc(buf, ncap);
			if(nb == NULL) {
				free(buf);
				return NULL;
			}
			buf = nb;
			cap = ncap;
		}
		ssize_t r;
		errno = 0;
		r = read(fd, buf + len, cap - len - 1);
		if(r < 0 && (errno == EAGAIN || errno == EINTR))
			continue;
		if(r < 0) {
			free(buf);
			return NULL;
		}
		if(r == 0)
			break;
		len += (size_t)r;
	}
	buf[len] = '\0';
	*out_len = len;
	return buf;
}

static void free_doc(void) {
	free(doc.data);
	free(doc.off);
	free(doc.len);
	memset(&doc, 0, sizeof(doc));
}

/* Load one input: a named file, or stdin when name is NULL. Returns 0 on
 * success; on failure prints a more(1)-style message and returns -1. */
static int load_input(const char* name) {
	free_doc();
	size_t len = 0;
	char* data;
	if(name != NULL) {
		int fd = open(name, O_RDONLY);
		if(fd < 0) {
			fprintf(stderr, "Cannot open %s (%s)\n", name, strerror(errno));
			return -1;
		}
		data = read_all_fd(fd, &len);
		close(fd);
	} else {
		data = read_all_fd(0, &len);
	}
	if(data == NULL) {
		fprintf(stderr, "Cannot read %s (%s)\n",
		        name ? name : "standard input", strerror(errno));
		return -1;
	}
	doc.data = data;
	doc.data_len = len;
	index_lines(&doc);
	return 0;
}

/* Line accessors. len[] is stored separately so squeezed lines, whose
 * neighbours are no longer adjacent in memory, still report the right size. */
static const char* line_ptr(long i) { return doc.data + doc.off[i]; }
static int line_len(long i) { return doc.len[i]; }

/* Temporarily NUL-terminate line i so regex/string helpers see just that
 * line, then restore the original terminator byte. */
static char* line_cstr(long i, char* saved) {
	char* s = doc.data + doc.off[i];
	int n = doc.len[i];
	*saved = s[n];
	s[n] = '\0';
	return s;
}
static void line_restore(long i, char saved) {
	doc.data[doc.off[i] + doc.len[i]] = saved;
}

/* =========================================================================
 *  Terminal geometry
 * ====================================================================== */

#define SIZE_PROBE_TIMEOUT_MS 200

/* Read the "ESC [ <row> ; <col> R" cursor-position report from the key fd. */
static bool read_cursor_pos_reply(int* rows, int* cols) {
	enum { WANT_ESC, WANT_BRACKET, WANT_PARAMS } state = WANT_ESC;
	int row = 0, col = 0;
	bool parsing_col = false;

	for(;;) {
		struct pollfd pfd;
		pfd.fd = key_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int wait_ms = (state == WANT_ESC) ? SIZE_PROBE_TIMEOUT_MS : 100;
		if(poll(&pfd, 1, wait_ms) <= 0)
			return false;

		char c;
		if(read(key_fd, &c, 1) != 1)
			return false;

		if(state == WANT_ESC) {
			if(c == 0x1b)
				state = WANT_BRACKET;
		} else if(state == WANT_BRACKET) {
			state = (c == '[') ? WANT_PARAMS : WANT_ESC;
		} else {
			if(c >= '0' && c <= '9') {
				if(parsing_col) col = col * 10 + (c - '0');
				else            row = row * 10 + (c - '0');
			} else if(c == ';') {
				parsing_col = true;
			} else if(c == 'R') {
				if(row > 0 && col > 0) {
					*rows = row;
					*cols = col;
					return true;
				}
				return false;
			} else {
				state = WANT_ESC;
				row = col = 0;
				parsing_col = false;
			}
		}
	}
}

/* allow_probe is true only once at start-up: the CPR query consumes stdin, so
 * it must never run while the user may be typing commands. */
static void update_screen_size(bool allow_probe) {
	int rows = 0, cols = 0;

	if(isatty(1)) {
		struct winsize ws;
		memset(&ws, 0, sizeof(ws));
		if(ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
			rows = ws.ws_row;
			cols = ws.ws_col;
		}
	}

	if(rows == 0 && allow_probe && isatty(1) && key_fd >= 0 && isatty(key_fd)) {
		fputs(ESC "[999;999H" ESC "[6n", stdout);
		fflush(stdout);
		int r = 0, c = 0;
		if(read_cursor_pos_reply(&r, &c)) {
			rows = r;
			cols = c;
		}
		fputs(ESC_HOME, stdout);
		fflush(stdout);
	}

	if(rows > 0) term_rows = rows;
	if(cols > 0) term_cols = cols;
	if(term_rows < 2) term_rows = 2;
	if(term_cols < 2) term_cols = 2;
}

static int page_rows(void) {
	int win = (o_lines > 0) ? o_lines : term_rows;
	if(win < 2) win = 2;
	return win - 1; /* bottom row is reserved for the status line */
}

/* =========================================================================
 *  Raw mode
 * ====================================================================== */

static void enable_raw(void) {
	if(key_fd < 0 || !isatty(key_fd))
		return;
	if(tcgetattr(key_fd, &saved_tio) == 0) {
		tio_saved = true;
		struct termios raw = saved_tio;
		/* Raw *input* only: deliver each key at once, with no echo and no
		 * signal chars. c_oflag is deliberately left untouched - the EwokOS
		 * consoles render a bare '\n' as CR+LF themselves (and tty_output
		 * expands it when the caller did enable ONLCR), so clearing OPOST
		 * here would only risk staircase output for no benefit. */
		raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ECHOE | ECHOK |
		                             ECHONL | ISIG | IEXTEN);
		raw.c_iflag &= ~(tcflag_t)(ICRNL | INLCR | IGNCR | IXON | ISTRIP);
		raw.c_cc[VMIN] = 1;
		raw.c_cc[VTIME] = 0;
		tcsetattr(key_fd, TCSANOW, &raw);
	}
}

static void disable_raw(void) {
	if(tio_saved && key_fd >= 0)
		tcsetattr(key_fd, TCSANOW, &saved_tio);
	tio_saved = false;
}

/* =========================================================================
 *  Key and line input
 * ====================================================================== */

static int read_key(void) {
	fflush(stdout);

	struct pollfd pfd;
	pfd.fd = key_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if(poll(&pfd, 1, -1) <= 0)
		return KEY_ABORT;

	unsigned char c;
	if(read(key_fd, &c, 1) != 1)
		return KEY_ABORT;
	if(c != 0x1b)
		return (int)c;

	/* An ESC may begin a cursor-key sequence; the rest arrives together.
	 * If nothing follows promptly it is a lone ESC (a quit command). */
	pfd.revents = 0;
	if(poll(&pfd, 1, 50) <= 0)
		return 0x1b;
	if(read(key_fd, &c, 1) != 1)
		return 0x1b;

	if(c == '[' || c == 'O') {
		if(read(key_fd, &c, 1) != 1)
			return 0x1b;
		switch(c) {
		case 'A': return KEY_UP;
		case 'B': return KEY_DOWN;
		case 'C': return KEY_RIGHT;
		case 'D': return KEY_LEFT;
		case 'H': return KEY_HOME;
		case 'F': return KEY_END;
		case '5': read(key_fd, &c, 1); return KEY_PGUP;
		case '6': read(key_fd, &c, 1); return KEY_PGDN;
		default:  return 0x1b;
		}
	}
	if(c == 'v')
		return KEY_ESC_V;
	return 0x1b;
}

/* Move the cursor to the status row (bottom line), column 1, and clear it. */
static void goto_status_row(void) {
	printf(ESC "[%d;1H" ESC_CLEAR2EOL, term_rows);
}

/* Read a one-line command (used by '/' and ':'), echoing on the status row.
 * Returns the string on ENTER, or NULL if the user cancelled with ESC. */
static char* read_line_input(const char* prompt) {
	static char line[256];
	int n = 0;
	line[0] = '\0';

	for(;;) {
		goto_status_row();
		fputs(prompt, stdout);
		fwrite(line, 1, (size_t)n, stdout);
		fflush(stdout);

		struct pollfd pfd;
		pfd.fd = key_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		if(poll(&pfd, 1, -1) <= 0)
			return NULL;

		char c;
		if(read(key_fd, &c, 1) != 1)
			return NULL;

		if(c == '\r' || c == '\n') {
			line[n] = '\0';
			return line;
		}
		if(c == 0x1b || c == CTRL('C')) {
			return NULL;
		}
		if(c == 0x7f || c == 0x08 || c == CTRL('U')) {
			if(c == CTRL('U')) n = 0;
			else if(n > 0) n--;
			continue;
		}
		if((unsigned char)c >= 0x20 && n < (int)sizeof(line) - 1) {
			line[n++] = c;
		}
	}
}

/* =========================================================================
 *  Rendering
 * ====================================================================== */

/* Display width of a line, expanding tabs to 8-column stops. Control bytes
 * take no width, matching how a terminal renders them. */
static int disp_width(const char* s, int len) {
	int col = 0;
	for(int i = 0; i < len; i++) {
		unsigned char c = (unsigned char)s[i];
		if(c == '\t')
			col = ((col >> 3) + 1) << 3;
		else if(c >= 0x20 && c != 0x7f)
			col++;
	}
	return col;
}

/* Screen rows a line occupies once wrapped at term_cols. With -f every
 * logical line counts as one row regardless of its length. */
static int line_height(long i) {
	if(o_logical)
		return 1;
	int w = disp_width(line_ptr(i), line_len(i));
	int h = (w + term_cols - 1) / term_cols;
	return h < 1 ? 1 : h;
}

/* Emit a line body. With -u, backspace overstrike sequences (used for bold
 * and underline) are stripped so the text comes out plain. */
static void emit_line_body(long i) {
	const char* s = line_ptr(i);
	int len = line_len(i);
	if(o_plain) {
		for(int j = 0; j < len; j++) {
			if(s[j] == '\b')
				continue;
			if(j + 1 < len && s[j + 1] == '\b')
				continue;
			fputc(s[j], stdout);
		}
	} else {
		fwrite(s, 1, (size_t)len, stdout);
	}
}

static void emit_line(long i) {
	emit_line_body(i);
	fputs(ESC_CLEAR2EOL, stdout);
	fputc('\n', stdout);
}

/* A single line taller than the remaining screen: emit just enough leading
 * bytes to fill avail_rows rows and clear the tail (no trailing newline). */
static void emit_line_clipped(long i, int avail_rows) {
	const char* s = line_ptr(i);
	int len = line_len(i);
	int budget = avail_rows * term_cols;
	int col = 0;
	for(int j = 0; j < len; j++) {
		unsigned char c = (unsigned char)s[j];
		int w;
		if(c == '\t')
			w = (((col >> 3) + 1) << 3) - col;
		else if(c >= 0x20 && c != 0x7f)
			w = 1;
		else
			w = 0;
		if(col + w > budget)
			break;
		col += w;
		fputc(s[j], stdout);
	}
	fputs(ESC_CLEAR2EOL, stdout);
}

/* Draw the screen starting at logical line `top`. Returns the index of the
 * first line that did not fit (== nlines when the end of file is shown). */
static long render_screen(long top) {
	update_screen_size(false);
	int prow = page_rows();

	fputs(ESC_HOME, stdout);

	long i = top;
	int rows = 0;
	while(i < doc.nlines && rows < prow) {
		int h = line_height(i);
		if(rows + h > prow) {
			emit_line_clipped(i, prow - rows);
			rows = prow;
			i++;
			break;
		}
		emit_line(i);
		rows += h;
		i++;
	}
	while(rows < prow) {
		fputs(ESC_CLEAR2EOL, stdout);
		fputc('\n', stdout);
		rows++;
	}
	return i;
}

/* One screenful forward, returning the next top line (used for navigation
 * without drawing so 'b' can step back by the same amount). */
static long next_screen_top(long top) {
	int prow = page_rows();
	long i = top;
	int rows = 0;
	while(i < doc.nlines && rows < prow) {
		int h = line_height(i);
		if(rows + h > prow) {
			i++;
			break;
		}
		rows += h;
		i++;
	}
	return i;
}

/* One screenful backward: step back until a screen from there would end at
 * or past the current top. */
static long prev_screen_top(long top) {
	int prow = page_rows();
	long t = top;
	int rows = 0;
	while(t > 0 && rows < prow) {
		long p = t - 1;
		int h = line_height(p);
		if(rows + h > prow)
			break;
		rows += h;
		t = p;
	}
	return t;
}

static const char* base_name(const char* p) {
	const char* s = strrchr(p, '/');
	return s ? s + 1 : p;
}

/* Display name of the current input: the file's base name, or "(stdin)" when
 * more is paging standard input (files[cur_file] is NULL). */
static const char* cur_name(void) {
	if(cur_file < nfiles && files[cur_file] != NULL)
		return base_name(files[cur_file]);
	return "(stdin)";
}

/* Build the default "--More--(NN%)" prompt, including the file name when
 * several files are queued and a next-file hint at end of file. */
static void build_prompt(char* buf, size_t sz, long next_top, bool eof) {
	char pos[64];
	pos[0] = '\0';

	if(eof && cur_file + 1 < nfiles) {
		snprintf(pos, sizeof(pos), "(next file: %s)", base_name(files[cur_file + 1]));
	} else {
		int pct = 100;
		if(doc.nlines > 0 && next_top < doc.nlines)
			pct = (int)((long)next_top * 100 / doc.nlines);
		snprintf(pos, sizeof(pos), "(%d%%)", pct);
	}

	if(o_silent) {
		if(nfiles > 1)
			snprintf(buf, sz, "%s[Press space to continue, 'q' to quit.]%s",
			         cur_name(), pos);
		else
			snprintf(buf, sz, "[Press space to continue, 'q' to quit.]%s", pos);
	} else if(nfiles > 1) {
		snprintf(buf, sz, "%s--More--%s", cur_name(), pos);
	} else {
		snprintf(buf, sz, "--More--%s", pos);
	}
}

/* Draw the bottom status row: a one-shot message if pending, else the prompt. */
static void draw_status(long next_top, bool eof) {
	char buf[300];
	if(pending_msg[0] != '\0')
		snprintf(buf, sizeof(buf), "%s", pending_msg);
	else
		build_prompt(buf, sizeof(buf), next_top, eof);

	goto_status_row();
	fputs(ESC_REVERSE, stdout);
	int n = (int)strlen(buf);
	if(n > term_cols - 1)
		n = term_cols - 1;
	fwrite(buf, 1, (size_t)n, stdout);
	fputs(ESC_CLEAR2EOL ESC_NORMAL, stdout);
	fflush(stdout);
}

static void ring_or_hint(void) {
	if(o_silent)
		snprintf(pending_msg, sizeof(pending_msg),
		         "[Press space to continue, 'q' to quit.]");
	else
		fputs(ESC_BELL, stdout);
}

/* =========================================================================
 *  Search
 * ====================================================================== */

static long search_from(long start, int forward, const char* pat) {
	regex_t re;
	if(regcomp(&re, pat, REG_EXTENDED | REG_NOSUB) != 0) {
		snprintf(pending_msg, sizeof(pending_msg), "Bad regular expression");
		return -1;
	}
	long found = -1;
	if(forward) {
		for(long i = start; i < doc.nlines; i++) {
			char saved;
			char* s = line_cstr(i, &saved);
			int m = regexec(&re, s, 0, NULL, 0);
			line_restore(i, saved);
			if(m == 0) {
				found = i;
				break;
			}
		}
	} else {
		for(long i = start; i >= 0; i--) {
			char saved;
			char* s = line_cstr(i, &saved);
			int m = regexec(&re, s, 0, NULL, 0);
			line_restore(i, saved);
			if(m == 0) {
				found = i;
				break;
			}
		}
	}
	regfree(&re);
	return found;
}

/* =========================================================================
 *  Help screen
 * ====================================================================== */

static const char* help_text[] = {
	"                 Most commands optionally preceded by integer argument k.",
	"                 Defaults for k are shown in parens.",
	"h  or  ?         this message",
	"b  or  ^B  or ESC-v   skip back k lines (one screen)",
	"RETURN or j or ^N     next k lines (1)",
	"SPACE or f or ^F      next k lines (one screen)",
	"d  or  ^D        scroll k lines (half screen)",
	"u  or  ^U        scroll back k lines (half screen)",
	"y  or  k  or ^Y  back k lines (1)",
	"q  or  Q  or ^C  exit",
	"s                skip k lines (1)",
	"'                go to start of the last search",
	"=                display the current line number",
	"/pattern         search forward for a regular expression",
	"n                repeat the last search",
	":n               go to the next file",
	":p               go to the previous file",
	":f               display the current file name and line number",
	":q               exit",
	"^L               redraw the screen",
	NULL
};

static void show_help(void) {
	fputs(ESC_HIDE_CURSOR ESC_CLEAR_SCREEN ESC_HOME, stdout);
	int prow = page_rows();
	int r = 0;
	for(int i = 0; help_text[i] != NULL && r < prow; i++, r++) {
		fputs(help_text[i], stdout);
		fputs(ESC_CLEAR2EOL, stdout);
		fputc('\n', stdout);
	}
	while(r < prow) {
		fputs(ESC_CLEAR2EOL, stdout);
		fputc('\n', stdout);
		r++;
	}
	goto_status_row();
	fputs(ESC_REVERSE "[Press any key to continue]" ESC_CLEAR2EOL ESC_NORMAL, stdout);
	fflush(stdout);
	read_key();
}

/* =========================================================================
 *  Colon commands (:n :p :f :q)
 * ====================================================================== */

/* Returns: 0 keep paging current file, 1 switch file (cur_file updated),
 * 2 quit. */
static int handle_colon(long top) {
	char* s = read_line_input(":");
	if(s == NULL) {
		pending_msg[0] = '\0';
		return 0;
	}
	/* skip an optional leading count, e.g. ":2n" */
	long k = 0;
	while(*s >= '0' && *s <= '9')
		k = k * 10 + (*s++ - '0');
	if(k == 0)
		k = 1;

	switch(*s) {
	case 'n': case 'N':
		if(cur_file + (int)k < nfiles) {
			cur_file += (int)k;
			return 1;
		}
		snprintf(pending_msg, sizeof(pending_msg), "No next file");
		return 0;
	case 'p': case 'P':
		if(cur_file - (int)k >= 0) {
			cur_file -= (int)k;
			return 1;
		}
		snprintf(pending_msg, sizeof(pending_msg), "No previous file");
		return 0;
	case 'f': case 'F':
		snprintf(pending_msg, sizeof(pending_msg), "%s line %ld",
		         cur_name(), top + 1);
		return 0;
	case 'q': case 'Q':
		return 2;
	default:
		snprintf(pending_msg, sizeof(pending_msg), "Unknown colon command");
		return 0;
	}
}

/* =========================================================================
 *  Non-interactive pass-through
 * ====================================================================== */

static void dump_all(void) {
	for(long i = 0; i < doc.nlines; i++) {
		fwrite(line_ptr(i), 1, (size_t)line_len(i), stdout);
		fputc('\n', stdout);
	}
	fflush(stdout);
}

/* =========================================================================
 *  The pager
 * ====================================================================== */

static long clamp_top(long t) {
	if(t < 0) t = 0;
	if(doc.nlines == 0) return 0;
	if(t > doc.nlines - 1) t = doc.nlines - 1;
	return t;
}

/* Page through the already-loaded document. Returns 0 normally, 1 to move to
 * the next file, 2 to quit everything. */
static int run_pager(long initial_top) {
	long top = clamp_top(initial_top);
	pending_msg[0] = '\0';
	have_count = false;
	pending_count = 0;

	fputs(ESC_HIDE_CURSOR, stdout);

	for(;;) {
		long next_top = render_screen(top);
		bool eof = (next_top >= doc.nlines);
		draw_status(next_top, eof);

		if(eof) {
			/* At end of file: advance to the next file if there is
			 * one, otherwise leave the final screen and return. */
			if(cur_file + 1 < nfiles) {
				int k = read_key();
				pending_msg[0] = '\0';
				if(k == 'q' || k == 'Q' || k == CTRL('C') || k == 0x1b)
					return 2;
				cur_file++;
				return 1;
			}
			return 0;
		}

		int key = read_key();
		if(key == KEY_ABORT)
			return 2;

		/* A numeric prefix accumulates until a command consumes it. A
		 * leading '0' is not a count (real more ignores it), so it falls
		 * through to the command switch like any other key. */
		if(key >= '0' && key <= '9' && !(pending_count == 0 && key == '0')) {
			pending_count = pending_count * 10 + (key - '0');
			have_count = true;
			snprintf(pending_msg, sizeof(pending_msg), "count: %ld",
			         pending_count);
			continue;
		}

		long count = have_count ? pending_count : 0;
		bool had_count = have_count;
		have_count = false;
		pending_count = 0;
		pending_msg[0] = '\0';

		switch(key) {
		case ' ': case 'f': case CTRL('F'): case CTRL('V'):
		case KEY_PGDN: case KEY_RIGHT:
			top = clamp_top(next_screen_top(top));
			for(long c = 1; c < (had_count ? count : 1) && top < doc.nlines; c++)
				top = clamp_top(next_screen_top(top));
			break;

		case 'z': {
			/* forward one screen and adopt it as the new default */
			long n = next_screen_top(top);
			top = clamp_top(n);
			break;
		}

		case '\r': case '\n': case 'j': case CTRL('N'):
		case KEY_DOWN: {
			long k = had_count ? count : 1;
			/* emulate scrolling: keep at least the last line visible */
			top = clamp_top(top + k);
			break;
		}

		case 'd': case CTRL('D'): {
			long k = had_count ? count : (page_rows() / 2 + 1);
			top = clamp_top(top + k);
			break;
		}

		case 'u': case CTRL('U'): {
			long k = had_count ? count : (page_rows() / 2 + 1);
			top = clamp_top(top - k);
			break;
		}

		case 'y': case 'k': case CTRL('Y'): case CTRL('K'): case KEY_UP:
		case KEY_LEFT: {
			long k = had_count ? count : 1;
			top = clamp_top(top - k);
			break;
		}

		case 'b': case CTRL('B'): case KEY_PGUP: case KEY_ESC_V: {
			long t = prev_screen_top(top);
			for(long c = 1; c < (had_count ? count : 1); c++)
				t = prev_screen_top(t);
			top = t;
			break;
		}

		case 's': {
			long k = had_count ? count : 1;
			top = clamp_top(top + k);
			break;
		}

		case '=': case CTRL('G'):
			snprintf(pending_msg, sizeof(pending_msg), "line %ld of %ld",
			         top + 1, doc.nlines);
			break;

		case '\'':
			if(search_mark >= 0)
				top = clamp_top(search_mark);
			else
				snprintf(pending_msg, sizeof(pending_msg), "No mark set");
			break;

		case '/': {
			char* pat = read_line_input("/");
			if(pat != NULL && pat[0] != '\0') {
				snprintf(search_pat, sizeof(search_pat), "%s", pat);
				have_search = true;
				search_mark = top;
				long f = search_from(top + 1, 1, search_pat);
				if(f >= 0)
					top = f;
				else
					snprintf(pending_msg, sizeof(pending_msg),
					         "Pattern not found");
			} else {
				pending_msg[0] = '\0';
			}
			break;
		}

		case '?': {
			/* backward search, as in less/more */
			char* pat = read_line_input("?");
			if(pat != NULL && pat[0] != '\0') {
				snprintf(search_pat, sizeof(search_pat), "%s", pat);
				have_search = true;
				search_mark = top;
				long f = search_from(top - 1, 0, search_pat);
				if(f >= 0)
					top = f;
				else
					snprintf(pending_msg, sizeof(pending_msg),
					         "Pattern not found");
			} else {
				pending_msg[0] = '\0';
			}
			break;
		}

		case 'n': {
			if(have_search) {
				search_mark = top;
				long f = search_from(top + 1, 1, search_pat);
				if(f >= 0)
					top = f;
				else
					snprintf(pending_msg, sizeof(pending_msg),
					         "Pattern not found");
			} else {
				snprintf(pending_msg, sizeof(pending_msg), "No previous search");
			}
			break;
		}

		case CTRL('L'): case 'r': case 'R':
			fputs(ESC_CLEAR_SCREEN, stdout);
			update_screen_size(false);
			break;

		case ':': {
			int r = handle_colon(top);
			if(r == 1) { disable_raw(); enable_raw(); return 1; }
			if(r == 2) return 2;
			break;
		}

		case 'h':
			show_help();
			fputs(ESC_CLEAR_SCREEN, stdout);
			break;

		case 'v':
			snprintf(pending_msg, sizeof(pending_msg),
			         "Editing not supported");
			break;

		case '!':
			snprintf(pending_msg, sizeof(pending_msg),
			         "Shell escapes not supported");
			break;

		case 'q': case 'Q': case CTRL('C'): case 0x1b:
			return 2;

		default:
			ring_or_hint();
			break;
		}
	}
}

/* Compute the first line to show for the current file, honouring +num and
 * +/pattern (applied to the first file only, as util-linux more does). */
static long initial_top_for_file(void) {
	if(cur_file == 0) {
		if(o_start_pat != NULL) {
			long f = search_from(0, 1, o_start_pat);
			if(f > 0)
				return f;
		}
		if(o_start_line > 1)
			return o_start_line - 1;
	}
	return 0;
}

static int page_all_inputs(void) {
	while(cur_file < nfiles) {
		const char* name = (nfiles > 0) ? files[cur_file] : NULL;
		if(load_input(name) != 0) {
			/* unreadable file: skip to the next one */
			cur_file++;
			continue;
		}

		long start = initial_top_for_file();
		if(!interactive || !isatty(1)) {
			dump_all();
			cur_file++;
			continue;
		}

		int r = run_pager(start);
		if(r == 2)
			break;                 /* user quit */
		if(r == 0)
			cur_file++;            /* finished this file, on to the next */
		/* r == 1 : cur_file already points at the next file to show */
	}
	return 0;
}

/* =========================================================================
 *  Usage / version
 * ====================================================================== */

static void usage(FILE* out) {
	fputs(
		"Usage: more [options] <file>...\n"
		"\n"
		"Options:\n"
		" -d, --silent          display help instead of ringing bell\n"
		" -f, --logical         count logical rather than screen lines\n"
		" -l, --no-pause        suppress pause after form feed\n"
		" -p, --print-over      do not scroll, clean screen and display text\n"
		" -c, --clean-print     do not scroll, display text and clean line ends\n"
		" -s, --squeeze         squeeze multiple blank lines into one\n"
		" -u, --plain           suppress underlining and bold\n"
		" -n, --lines <number>  the number of lines per screenful\n"
		"     --number <number> same as -n\n"
		" -V, --version         display version information and exit\n"
		" -h, --help            display this help text and exit\n"
		"\n"
		"Arguments:\n"
		" +<number>             start at the given line number\n"
		" +/<pattern>           start at the line containing the given pattern\n"
		" <file>...             files to display, or standard input when omitted\n"
		"\n"
		"For more details see more(1).\n", out);
}

/* =========================================================================
 *  Argument parsing
 * ====================================================================== */

static const struct option longopts[] = {
	{ "silent",      no_argument,       NULL, 'd' },
	{ "logical",     no_argument,       NULL, 'f' },
	{ "no-pause",    no_argument,       NULL, 'l' },
	{ "print-over",  no_argument,       NULL, 'p' },
	{ "clean-print", no_argument,       NULL, 'c' },
	{ "squeeze",     no_argument,       NULL, 's' },
	{ "plain",       no_argument,       NULL, 'u' },
	{ "lines",       required_argument, NULL, 'n' },
	{ "number",      required_argument, NULL, 'n' },
	{ "version",     no_argument,       NULL, 'V' },
	{ "help",        no_argument,       NULL, 'h' },
	{ NULL, 0, NULL, 0 }
};

/* Parse leading options. Returns 0 on success, -1 to exit(1) after printing
 * usage on stderr, 1 to exit(0) (help/version already printed). getopt_long
 * stops at the first non-option, leaving +args and file names in argv[optind..].
 */
static int parse_args(int argc, char** argv) {
	int c;
	while((c = getopt_long(argc, argv, "dflpcsun:Vh", longopts, NULL)) != -1) {
		switch(c) {
		case 'd': o_silent = true; break;
		case 'f': o_logical = true; break;
		case 'l': o_nopause = true; break;
		case 'p': o_printover = true; break;
		case 'c': o_cleanprint = true; break;
		case 's': o_squeeze = true; break;
		case 'u': o_plain = true; break;
		case 'n':
			o_lines = atoi(optarg);
			if(o_lines < 1) o_lines = 1;
			break;
		case 'V':
			printf("%s\n", MORE_VERSION);
			return 1;
		case 'h':
			usage(stdout);
			return 1;
		case '?':
		default:
			usage(stderr);
			return -1;
		}
	}

	/* Remaining args: +num / +/pattern (start position) and file names. */
	for(int i = optind; i < argc; i++) {
		char* a = argv[i];
		if(a[0] == '+' && a[1] != '\0') {
			if(a[1] == '/') {
				o_start_pat = a + 2;
			} else if(isdigit((unsigned char)a[1])) {
				o_start_line = atol(a + 1);
			} else {
				/* treat anything else as a file name */
				files = (char**)realloc(files, sizeof(char*) * (size_t)(nfiles + 1));
				files[nfiles++] = a;
			}
		} else {
			files = (char**)realloc(files, sizeof(char*) * (size_t)(nfiles + 1));
			files[nfiles++] = a;
		}
	}
	return 0;
}

/* =========================================================================
 *  main
 * ====================================================================== */

int main(int argc, char** argv) {
	int pr = parse_args(argc, argv);
	if(pr != 0)
		return (pr < 0) ? 1 : 0;

	/* No files: page standard input (still one "input"). */
	if(nfiles == 0) {
		files = (char**)malloc(sizeof(char*));
		files[0] = NULL;
		nfiles = 1;
	}

	bool out_tty = isatty(1) ? true : false;

	/* Choose a key source. With file arguments fd0 is untouched and, if it
	 * is a terminal, supplies keys. Otherwise (piped/redirected stdin) try
	 * to reopen the output terminal by name so `cmd | more` can still page.
	 */
	bool stdin_is_content = (files[0] == NULL);
	if(out_tty) {
		if(!stdin_is_content && isatty(0)) {
			key_fd = 0;
		} else {
			char* tn = isatty(1) ? ttyname(1) : NULL;
			if(tn != NULL) {
				int fd = open(tn, O_RDONLY);
				if(fd >= 0 && isatty(fd))
					key_fd = fd;
				else if(fd >= 0)
					close(fd);
			}
		}
	}
	interactive = out_tty && (key_fd >= 0);

	int rc = 0;
	if(interactive) {
		update_screen_size(true);
		enable_raw();
		fputs(ESC_HIDE_CURSOR, stdout);
		rc = page_all_inputs();
		disable_raw();
		fputs(ESC_SHOW_CURSOR, stdout);
		goto_status_row();
		fputs(ESC_CLEAR2EOL, stdout);
		fputc('\n', stdout);
		fflush(stdout);
	} else {
		/* pass-through: no paging, no escape sequences */
		rc = page_all_inputs();
	}

	free_doc();
	free(files);
	if(key_fd > 0)
		close(key_fd);
	return rc;
}
