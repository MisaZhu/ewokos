/* vim_util.c — small helpers and all global state (split from vi.c) */
#include "vim.h"

jmp_buf die_jmp UDATA;

void puts_no_eol(const char* s) {
    while (*s)
        putchar(*s++);
}

int index_in_strings(const char* strings, const char* key) {
    int j, idx = 0;

    while (*strings) {
        /* Do we see "key\0" at current position in strings? */
        for (j = 0; *strings == key[j]; ++j) {
            if (*strings++ == '\0') {
                // bb_error_msg("found:'%s' i:%u", key, idx);
                return idx; /* yes */
            }
        }
        /* No.  Move to the start of the next string. */
        while (*strings++ != '\0')
            continue;
        idx++;
    }
    return -1;
}

void* zalloc(size_t bytes) {
    char* cp = (char*)malloc(bytes);
    if (cp)
        memset(cp, 0, bytes);
    return cp;
}

char* strchrnul(const char* s, int c) {
    while (*s != '\0' && *s != c)
        s++;
    return (char*)s;
}

// macOS libc has no memrchr() (a GNU/glibc extension). Provide a portable
// one: scan the n bytes at s backwards for c, returning the last match or
// NULL, matching the glibc semantics the callers rely on.
void* memrchr(const void* s, int c, size_t n) {
    const unsigned char* p = (const unsigned char*)s + n;
    while (n--)
        if (*--p == (unsigned char)c)
            return (void*)p;
    return NULL;
}

const char* msg_memory_exhausted = "out of memory";

__attribute__((__noreturn__)) void error_msg_and_die(const char* s, ...) {
    va_list p;

    va_start(p, s);
    vfprintf(stdout, s, p); // ewokos libc has no vprintf()
    va_end(p);
    putchar('\n');
    longjmp(die_jmp, 1);
}

char* xvsnprintf(const char* format, ...) {
    char c;
    va_list va;
    va_start(va, format);
    int n = vsnprintf(&c, sizeof c, format, va);
    va_end(va);
    char* buf = malloc(n + 1);
    va_start(va, format);
    vsnprintf(buf, n + 1, format, va);
    va_end(va);
    return buf;
}

// cmds modifying text[]
const char modifying_cmds[] = "aAcCdDiIJoOpPrRsxX<>~";

//----- Globals ------------------------------------------------------
// many references - keep near the top of globals
char *text UDATA, *end UDATA; // pointers to the user data in memory
char* dot UDATA;              // where all the action takes place
int text_size UDATA;          // size of the allocated buffer

// the rest
int16_t vi_setops UDATA;               // set by setops()
int16_t editing UDATA;                 // >0 while we are editing a file
                                       // [code audit says "can be 0, 1 or 2 only"]
int16_t cmd_mode UDATA;                // 0=command  1=insert 2=replace
int modified_count UDATA;              // buffer contents changed if !0
int last_modified_count UDATA;         // = -1;
int cmdcnt UDATA;                      // repetition count
uint32_t rows UDATA, columns UDATA;    // the terminal screen is this size
int crow UDATA, ccol UDATA;            // cursor is on Crow x Ccol
int offset UDATA;                      // chars scrolled off the screen to the left
int have_status_msg UDATA;             // is default edit status needed?
                                       // [don't make int16_t!]
int last_status_cksum UDATA;           // hash of current status line
char* current_filename UDATA;
char* screenbegin UDATA; // index into text[], of top line on the screen
char* screen UDATA;      // pointer to the virtual screen buffer
int screensize UDATA;    //            and its size
int tabstop UDATA;
int last_search_char UDATA;    // last char searched for (int because of Unicode)
int16_t last_search_cmd UDATA; // command used to invoke last char search
char last_input_char UDATA;    // last char read from user
char undo_queue_state UDATA;   // One of UNDO_INS, UNDO_DEL, UNDO_EMPTY

int16_t adding2q UDATA;            // are we currently adding user input to q
int lmc_len UDATA;                 // length of last_modifying_cmd
char *ioq UDATA, *ioq_start UDATA; // pointer to string for get_one_char to "read"
int dotcnt UDATA;                  // number of times to repeat '.' command
char* last_search_pattern UDATA;   // last pattern from a '/' or '?' search
int indentcol UDATA;               // column of recently autoindent, 0 or -1
int16_t cmd_error UDATA;

int16_t vi_visual UDATA;      // 1 while a visual ('v') selection is active
char* vi_visual_anchor UDATA; // position where 'v' was pressed

// former statics
char* edit_file_cur_line UDATA;
int refresh_old_offset UDATA;
int format_edit_status_tot UDATA;

// a few references only
uint16_t YDreg UDATA;      // default delete register (Ureg = 27)
char* reg[28] UDATA;    // named register a-z, "D", and "U" 0-25,26,27
char regtype[28] UDATA; // buffer type: WHOLE, MULTI or PARTIAL
char* mark[28] UDATA; // user marks points somewhere in text[]-  a-z and previous context ''
int cindex UDATA;     // saved character index for up/down motion
int16_t keep_index UDATA; // retain saved character index
char readbuffer[KEYCODE_BUFFER_SIZE] UDATA;
char status_buffer[STATUS_BUFFER_LEN] UDATA;  // messages to the user
char last_modifying_cmd[MAX_INPUT_LEN] UDATA; // last modifying cmd for "."
char get_input_line_buf[MAX_INPUT_LEN] UDATA; // former static

char scr_out_buf[MAX_SCR_COLS + MAX_TABSTOP * 2] UDATA;

struct undo_object* undo_stack_tail UDATA;
char* undo_queue_spos UDATA; // Start position of queued operation
int undo_q UDATA;
char undo_queue[VI_UNDO_QUEUE_MAX] UDATA;

int argc UDATA, vi_optind UDATA;
