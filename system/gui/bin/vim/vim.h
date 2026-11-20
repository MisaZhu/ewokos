/* vim.h — shared constants, globals and prototypes for the vim editor.
 *
 * Split from system/basic/bin/vi/vi.c:
 *   tiny vi.c: A small 'vi' clone
 *   Copyright (C) 2000, 2001 Sterling Huxley <sterling@europa.com>
 *   Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *   Adapted for Raspberry Pi, 2021 lurk101
 *
 * This version adds JSON / .rd / .conf / .js / C syntax highlighting
 * (see vim_syntax.c) and a charwise visual selection mode ('v').
 */
#pragma once

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>

#define ARRAY_SIZE(x) ((uint32_t)(sizeof(x) / sizeof((x)[0])))

#define VI_VER "0.9.1-vim"

// The EwokOS build gathered these globals into a custom ".viudata" linker
// section. That section is meaningless on a host Mach-O/ELF build, so UDATA
// is a no-op here.
#define UDATA

/* "Keycodes" that report an escape sequence.
 * We use something which fits into signed char,
 * yet doesn't represent any valid Unicode character.
 * Also, -1 is reserved for error indication and we don't use it. */
/* clang-format off */
enum {
    KEYCODE_UP = -2,
    KEYCODE_DOWN = -3,
    KEYCODE_RIGHT = -4,
    KEYCODE_LEFT = -5,
    KEYCODE_HOME = -6,
    KEYCODE_END = -7,
    KEYCODE_INSERT = -8,
    KEYCODE_DELETE = -9,
    KEYCODE_PAGEUP = -10,
    KEYCODE_PAGEDOWN = -11,
    KEYCODE_BACKSPACE = -12, /* Used only if Alt/Ctrl/Shifted */
    KEYCODE_D = -13,         /* Used only if Alted */
    KEYCODE_CTRL_RIGHT = KEYCODE_RIGHT & ~0x40,
    KEYCODE_CTRL_LEFT = KEYCODE_LEFT & ~0x40,
    KEYCODE_ALT_RIGHT = KEYCODE_RIGHT & ~0x20,
    KEYCODE_ALT_LEFT = KEYCODE_LEFT & ~0x20,
    KEYCODE_ALT_BACKSPACE = KEYCODE_BACKSPACE & ~0x20,
    KEYCODE_ALT_D = KEYCODE_D & ~0x20,

    KEYCODE_CURSOR_POS = -0x100, /* 0xfff..fff00 */
    KEYCODE_BUFFER_SIZE = 16
};
/* clang-format on */

#define VI_MAX_SCREEN_LEN 4096
#define VI_UNDO_QUEUE_MAX 32

#define is_asciionly(a) ((uint32_t)((a)-0x20) <= 0x7e - 0x20)

enum {
    MAX_TABSTOP = 32, // sanity limit
    // User input len. Need not be extra big.
    // Lines in file being edited *can* be bigger than this.
    MAX_INPUT_LEN = 128,
    // Sanity limits. We have only one buffer of this size.
    MAX_SCR_COLS = VI_MAX_SCREEN_LEN,
    MAX_SCR_ROWS = VI_MAX_SCREEN_LEN,
};

// VT102 ESC sequences.
// See "Xterm Control Sequences"
#define ESC "\033"
// Inverse/Normal text
#define ESC_BOLD_TEXT ESC "[7m"
#define ESC_NORM_TEXT ESC "[m"
// Bell
#define ESC_BELL "\007"
// Clear-to-end-of-line
#define ESC_CLEAR2EOL ESC "[K"
// Clear-to-end-of-screen.
// (We use default param here.
// Full sequence is "ESC [ <num> J",
// <num> is 0/1/2 = "erase below/above/all".)
#define ESC_CLEAR2EOS ESC "[J"
// Cursor to given coordinate (1,1: top left)
#define ESC_SET_CURSOR_POS ESC "[%u;%uH"
#define ESC_SET_CURSOR_TOPLEFT ESC "[H"
// Hide/show the cursor (VT100 DECTCEM), used while repainting
#define ESC_CURSOR_HIDE ESC "[?25l"
#define ESC_CURSOR_SHOW ESC "[?25h"

// cmds modifying text[]
extern const char modifying_cmds[];

enum {
    YANKONLY = false,
    YANKDEL = true,
    FORWARD = 1, // code depends on "1"  for array index
    BACK = -1,   // code depends on "-1" for array index
    LIMITED = 0, // char_search() only current line
    FULL = 1,    // char_search() to the end/beginning of entire text
    PARTIAL = 0, // buffer contains partial line
    WHOLE = 1,   // buffer contains whole lines
    MULTI = 2,   // buffer may include newlines

    S_BEFORE_WS = 1, // used in skip_thing() for moving "dot"
    S_TO_WS = 2,     // used in skip_thing() for moving "dot"
    S_OVER_WS = 3,   // used in skip_thing() for moving "dot"
    S_END_PUNCT = 4, // used in skip_thing() for moving "dot"
    S_END_ALNUM = 5, // used in skip_thing() for moving "dot"

    C_END = -1, // cursor is at end of line due to '$' command
};

// set by setops()
#define VI_AUTOINDENT (1 << 0)
#define VI_EXPANDTAB (1 << 1)
#define VI_ERR_METHOD (1 << 2)
#define VI_IGNORECASE (1 << 3)
#define VI_SHOWMATCH (1 << 4)
#define VI_TABSTOP (1 << 5)
#define autoindent (vi_setops & VI_AUTOINDENT)
#define expandtab (vi_setops & VI_EXPANDTAB)
#define err_method (vi_setops & VI_ERR_METHOD) // indicate error with beep or flash
#define ignorecase (vi_setops & VI_IGNORECASE)
#define showmatch (vi_setops & VI_SHOWMATCH)
// order of constants and strings must match
#define OPTS_STR                                                                                   \
    "ai\0"                                                                                         \
    "autoindent\0"                                                                                 \
    "et\0"                                                                                         \
    "expandtab\0"                                                                                  \
    "fl\0"                                                                                         \
    "flash\0"                                                                                      \
    "ic\0"                                                                                         \
    "ignorecase\0"                                                                                 \
    "sm\0"                                                                                         \
    "showmatch\0"                                                                                  \
    "ts\0"                                                                                         \
    "tabstop\0"

#define SET_READONLY_FILE(flags) ((void)0)
#define SET_READONLY_MODE(flags) ((void)0)
#define UNSET_READONLY_FILE(flags) ((void)0)

#define Ureg 27
#define STATUS_BUFFER_LEN 200

// undo_push() operations
#define UNDO_INS 0
#define UNDO_DEL 1
#define UNDO_INS_CHAIN 2
#define UNDO_DEL_CHAIN 3
#define UNDO_INS_QUEUED 4
#define UNDO_DEL_QUEUED 5

// Pass-through flags for functions that can be undone
#define NO_UNDO 0
#define ALLOW_UNDO 1
#define ALLOW_UNDO_CHAIN 2
#define ALLOW_UNDO_QUEUED 3

struct undo_object {
    struct undo_object* prev; // Linking back avoids list traversal (LIFO)
    int start;                // Offset where the data should be restored/deleted
    int length;               // total data size
    uint8_t u_type;           // 0=deleted, 1=inserted, 2=swapped
    char undo_text[1];        // text that was deleted (if deletion)
};
#define UNDO_USE_SPOS 32
#define UNDO_EMPTY 64

//----- Globals (defined in vim_util.c) ------------------------------
extern jmp_buf die_jmp;
extern const char* msg_memory_exhausted;

// many references - keep near the top of globals
extern char *text, *end; // pointers to the user data in memory
extern char* dot;        // where all the action takes place
extern int text_size;    // size of the allocated buffer

extern int16_t vi_setops; // set by setops()
extern int16_t editing;   // >0 while we are editing a file
extern int16_t cmd_mode;  // 0=command  1=insert 2=replace
extern int modified_count;
extern int last_modified_count;
extern int cmdcnt;                   // repetition count
extern uint32_t rows, columns;       // the terminal screen is this size
extern int crow, ccol;               // cursor is on Crow x Ccol
extern int offset;                   // chars scrolled off the screen to the left
extern int have_status_msg;          // is default edit status needed?
extern int last_status_cksum;        // hash of current status line
extern char* current_filename;
extern char* screenbegin; // index into text[], of top line on the screen
extern char* screen;      // pointer to the virtual screen buffer
extern int screensize;    //            and its size
extern int tabstop;
extern int last_search_char;    // last char searched for (int because of Unicode)
extern int16_t last_search_cmd; // command used to invoke last char search
extern char last_input_char;    // last char read from user
extern char undo_queue_state;   // One of UNDO_INS, UNDO_DEL, UNDO_EMPTY

extern int16_t adding2q;          // are we currently adding user input to q
extern int lmc_len;               // length of last_modifying_cmd
extern char *ioq, *ioq_start;     // pointer to string for get_one_char to "read"
extern int dotcnt;                // number of times to repeat '.' command
extern char* last_search_pattern; // last pattern from a '/' or '?' search
extern int indentcol;             // column of recently autoindent, 0 or -1
extern int16_t cmd_error;

extern int16_t vi_visual;      // 1 while a visual ('v') selection is active
extern char* vi_visual_anchor; // position where 'v' was pressed

extern char* edit_file_cur_line;
extern int refresh_old_offset;
extern int format_edit_status_tot;

extern uint16_t YDreg; // default delete register (Ureg = 27)
extern char* reg[28];    // named register a-z, "D", and "U" 0-25,26,27
extern char regtype[28]; // buffer type: WHOLE, MULTI or PARTIAL
extern char* mark[28]; // user marks points somewhere in text[]-  a-z and previous context ''
extern int cindex;     // saved character index for up/down motion
extern int16_t keep_index; // retain saved character index
extern char readbuffer[KEYCODE_BUFFER_SIZE];
extern char status_buffer[STATUS_BUFFER_LEN];  // messages to the user
extern char last_modifying_cmd[MAX_INPUT_LEN]; // last modifying cmd for "."
extern char get_input_line_buf[MAX_INPUT_LEN]; // former static

extern char scr_out_buf[MAX_SCR_COLS + MAX_TABSTOP * 2];

extern struct undo_object* undo_stack_tail;
extern char* undo_queue_spos; // Start position of queued operation
extern int undo_q;
extern char undo_queue[VI_UNDO_QUEUE_MAX];

extern int argc, vi_optind;

//----- Prototypes ---------------------------------------------------
void puts_no_eol(const char* s);
int index_in_strings(const char* strings, const char* key);
void* zalloc(size_t bytes);
char* strchrnul(const char* s, int c);
void* memrchr(const void* s, int c, size_t n); // GNU ext, absent from macOS libc
__attribute__((__noreturn__)) void error_msg_and_die(const char* s, ...);
char* xvsnprintf(const char* format, ...);
void place_cursor(int row, int col);
// keep the cursor out of sight while rows are being repainted
void hide_cursor(void);
void show_cursor(void);
void clear_to_eol(void);
void go_bottom_and_clear_to_eol(void);
void standout_start(void);
void standout_end(void);
char* begin_line(char* p);
char* end_line(char* p);
char* dollar_line(char* p);
char* prev_line(char* p);
char* next_line(char* p);
char* end_screen(void);
int count_lines(char* start, char* stop);
char* find_line(int li);
int next_tabstop(int col);
int prev_tabstop(int col);
int next_column(char c, int co);
int get_column(char* p);
void screen_erase(void);
void new_screen(int ro, int co);
void sync_cursor(char* d, int* row, int* col);
char* format_line(char* src /*, int li*/);
void refresh(int full_screen);
// force a redraw of the screen columns a highlight change touches in [a, b]
void visual_invalidate_span(char* a, char* b);
int safe_poll(uint8_t* buffer, int timeout);
int read_key(char* buffer, int timeout);
int readit(void);
int get_one_char(void);
int get_motion_char(void);
char* get_input_line(const char* prompt);
int format_edit_status(void);
int bufsum(char* buf, int count);
void Hit_Return(void);
void show_status_line(void);
void redraw(int full_screen);
void flash(int ms);
void indicate_error(void);
void status_line(const char* format, ...);
void status_line_bold(const char* format, ...);
void status_line_bold_errno(const char* fn);
void print_literal(char* buf, const char* s);
void not_implemented(const char* s);
char* text_yank(char* p, char* q, int dest, int buftype);
char what_reg(void);
void check_context(char cmd);
char* swap_context(char* p);
void yank_status(const char* op, const char* p, int cnt);
intptr_t text_hole_make(char* p, int size);
char* text_hole_delete(char* p, char* q, int undo);
void undo_queue_commit(void);
void undo_push(char* src, uint32_t length, int u_type);
void flush_undo_data(void);
void undo_push_insert(char* p, int len, int undo);
intptr_t string_insert(char* p, const char* s, int undo);
void undo_pop(void);
void dot_left(void);
void dot_right(void);
void dot_begin(void);
void dot_end(void);
char* move_to_col(char* p, int l);
void dot_next(void);
void dot_prev(void);
void dot_skip_over_ws(void);
void dot_to_char(int cmd);
void dot_scroll(int cnt, int dir);
char* bound_dot(char* p);
void start_new_cmd_q(char c);
void end_cmd_q(void);
char* yank_delete(char* start, char* stop, int buftype, int yf, int undo);
int file_insert(const char* fn, char* p, int initial);
char* find_pair(char* p, const char c);
void showmatching(char* p);
intptr_t stupid_insert(char* p, char c);
size_t indent_len(char* p);
char* char_insert(char* p, char c, int undo);
void init_filename(char* fn);
void update_filename(char* fn);
int init_text_buffer(char* fn);
int file_write(char* fn, char* first, char* last);
int mycmp(const char* s1, const char* s2, int len);
char* char_search(char* p, const char* pat, int dir_and_range);
char* get_one_address(char* p, int* result, int* valid);
char* get_address(char* p, int* b, int* e, uint32_t* got);
void setops(char* args, int flg_no);
char* skip_whitespace(const char* s);
char* skip_non_whitespace(const char* s);
void colon(char* buf);
int st_test(char* p, int type, int dir, char* tested);
char* skip_thing(char* p, int linecnt, int dir, int type);
int at_eof(const char* s);
int find_range(char** start, char** stop, int cmd);
void do_cmd(int c);
void run_cmds(char* p);
void edit_file(char* fn);
void* xmalloc_open_read_close(const char* filename);
bool read_cursor_pos_reply(uint32_t* rows, uint32_t* cols);
void get_screen_xy(uint32_t* x, uint32_t* y);
