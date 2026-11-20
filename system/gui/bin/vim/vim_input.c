/* vim_input.c — keyboard input handling (split from vi.c) */
#include "vim.h"

/* How long to wait for the tail of an ESC sequence once its leading ESC has
 * been seen. A terminal emits a sequence as one unit, so this only has to
 * bridge sequences split up by a slow serial or telnet link. Keeping it short
 * is what makes a lone ESC (leave insert mode, abandon a 'v' selection) show
 * up at once instead of hanging until the next keypress. */
#define ESC_SEQ_TIMEOUT_MS 50

/* Read one byte from stdin into *buffer.
 * timeout < 0 blocks for the next keypress, 0 accepts only input that is
 * already available, > 0 waits at most that many milliseconds.
 * Returns 1 on success, 0 when no input arrived in time, -1 on error.
 *
 * Only the bounded waits go through poll(): the blocking read is the one path
 * every terminal is known to support, and a driver that reports no readiness
 * must not turn the main key loop into a spin. */
int safe_poll(uint8_t* buffer, int timeout) {
    if (timeout >= 0) {
        struct pollfd pfd;

        pfd.fd = 0; // stdin
        pfd.events = POLLIN;
        pfd.revents = 0;
        if (poll(&pfd, 1, timeout) <= 0)
            return 0; // nothing available within the time limit
    }
    return (read(0, buffer, 1) == 1) ? 1 : -1;
}

/* Known escape sequences for cursor and function keys.
 * See "Xterm Control Sequences"
 * http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * Array should be sorted from shortest to longest.
 */
static const char esccmds[] = {
    '\x7f' | 0x80, KEYCODE_ALT_BACKSPACE, '\b' | 0x80, KEYCODE_ALT_BACKSPACE, 'd' | 0x80,
    KEYCODE_ALT_D,
    /* lineedit mimics bash: Alt-f and Alt-b are forward/backward
     * word jumps. We cheat here and make them return ALT_LEFT/RIGHT
     * keycodes. This way, lineedit need no special code to handle them.
     * If we'll need to distinguish them, introduce new ALT_F/B keycodes,
     * and update lineedit to react to them.
     */
    'f' | 0x80, KEYCODE_ALT_RIGHT, 'b' | 0x80, KEYCODE_ALT_LEFT, 'O', 'A' | 0x80, KEYCODE_UP, 'O',
    'B' | 0x80, KEYCODE_DOWN, 'O', 'C' | 0x80, KEYCODE_RIGHT, 'O', 'D' | 0x80, KEYCODE_LEFT, 'O',
    'H' | 0x80, KEYCODE_HOME, 'O', 'F' | 0x80, KEYCODE_END, '[', 'A' | 0x80, KEYCODE_UP, '[',
    'B' | 0x80, KEYCODE_DOWN, '[', 'C' | 0x80, KEYCODE_RIGHT, '[', 'D' | 0x80, KEYCODE_LEFT,
    /* ESC [ 1 ; 2 x, where x = A/B/C/D: Shift-<arrow> */
    /* ESC [ 1 ; 3 x, where x = A/B/C/D: Alt-<arrow> - implemented below */
    /* ESC [ 1 ; 4 x, where x = A/B/C/D: Alt-Shift-<arrow> */
    /* ESC [ 1 ; 5 x, where x = A/B/C/D: Ctrl-<arrow> - implemented below */
    /* ESC [ 1 ; 6 x, where x = A/B/C/D: Ctrl-Shift-<arrow> */
    /* ESC [ 1 ; 7 x, where x = A/B/C/D: Ctrl-Alt-<arrow> */
    /* ESC [ 1 ; 8 x, where x = A/B/C/D: Ctrl-Alt-Shift-<arrow> */
    '[', 'H' | 0x80, KEYCODE_HOME, /* xterm */
    '[', 'F' | 0x80, KEYCODE_END,  /* xterm */
    /* [ESC] ESC [ [2] H - [Alt-][Shift-]Home (End similarly?) */
    /* '[','Z'        |0x80,KEYCODE_SHIFT_TAB, */
    '[', '1', '~' | 0x80, KEYCODE_HOME, /* vt100? linux vt? or what? */
    '[', '2', '~' | 0x80, KEYCODE_INSERT,
    /* ESC [ 2 ; 3 ~ - Alt-Insert */
    '[', '3', '~' | 0x80, KEYCODE_DELETE,
    /* [ESC] ESC [ 3 [;2] ~ - [Alt-][Shift-]Delete */
    /* ESC [ 3 ; 3 ~ - Alt-Delete */
    /* ESC [ 3 ; 5 ~ - Ctrl-Delete */
    '[', '4', '~' | 0x80, KEYCODE_END, /* vt100? linux vt? or what? */
    '[', '5', '~' | 0x80, KEYCODE_PAGEUP,
    /* ESC [ 5 ; 3 ~ - Alt-PgUp */
    /* ESC [ 5 ; 5 ~ - Ctrl-PgUp */
    /* ESC [ 5 ; 7 ~ - Ctrl-Alt-PgUp */
    '[', '6', '~' | 0x80, KEYCODE_PAGEDOWN, '[', '7', '~' | 0x80,
    KEYCODE_HOME,                      /* vt100? linux vt? or what? */
    '[', '8', '~' | 0x80, KEYCODE_END, /* vt100? linux vt? or what? */
    /* '[','1',';','5','A' |0x80,KEYCODE_CTRL_UP   , - unused */
    /* '[','1',';','5','B' |0x80,KEYCODE_CTRL_DOWN , - unused */
    '[', '1', ';', '5', 'C' | 0x80, KEYCODE_CTRL_RIGHT, '[', '1', ';', '5', 'D' | 0x80,
    KEYCODE_CTRL_LEFT,
    /* '[','1',';','3','A' |0x80,KEYCODE_ALT_UP    , - unused */
    /* '[','1',';','3','B' |0x80,KEYCODE_ALT_DOWN  , - unused */
    '[', '1', ';', '3', 'C' | 0x80, KEYCODE_ALT_RIGHT, '[', '1', ';', '3', 'D' | 0x80,
    KEYCODE_ALT_LEFT,
    /* '[','3',';','3','~' |0x80,KEYCODE_ALT_DELETE, - unused */
    0};

int read_key(char* buffer, int timeout) {
    const char* seq;
    int n, c;

    buffer++; /* saved chars counter is in buffer[-1] now */

start_over:
    errno = 0;
    n = (unsigned char)buffer[-1];
    if (n == 0) {
        /* If no data, wait for input.
         * If requested, wait TIMEOUT ms. TIMEOUT = -1 is useful
         * if fd can be in non-blocking mode.
         *
         * It is tempting to read more than one byte here,
         * but it breaks pasting. Example: at shell prompt,
         * user presses "c","a","t" and then pastes "\nline\n".
         * When we were reading 3 bytes here, we were eating
         * "li" too, and cat was getting wrong input.
         */
        n = safe_poll(buffer, timeout);
        if (n <= 0) {
            return -1;
        }
    }

    {
        unsigned char c = buffer[0];
        n--;
        if (n)
            memmove(buffer, buffer + 1, n);
        /* Only ESC starts ESC sequences */
        if (c != 27) {
            buffer[-1] = n;
            return c;
        }
    }

    /* Loop through known ESC sequences */
    seq = esccmds;
    while (*seq != '\0') {
        /* n - position in sequence we did not read yet */
        int i = 0; /* position in sequence to compare */

        /* Loop through chars in this sequence */
        while (1) {
            /* So far escape sequence matched up to [i-1] */
            if (n <= i) {
                /* Need more chars, read another one if it wouldn't block.
                 * Note that escape sequences come in as a unit,
                 * so if we block for long it's not really an escape sequence.
                 * Timeout is needed to reconnect escape sequences
                 * split up by transmission over a serial console. */
                errno = 0;
                if (safe_poll(buffer + n, ESC_SEQ_TIMEOUT_MS) <= 0) {
                    /* No more data!
                     * Array is sorted from shortest to longest,
                     * we can't match anything later in array -
                     * anything later is longer than this seq.
                     * Break out of both loops. */
                    if (n == 0)
                        return 27;
                    return -1;
                }
                n++;
            }
            if (buffer[i] != (seq[i] & 0x7f)) {
                /* This seq doesn't match, go to next */
                seq += i;
                /* Forward to last char */
                while (!(*seq & 0x80))
                    seq++;
                /* Skip it and the keycode which follows */
                seq += 2;
                break;
            }
            if (seq[i] & 0x80) {
                /* Entire seq matched */
                n = 0;
                /* n -= i; memmove(...);
                 * would be more correct,
                 * but we never read ahead that much,
                 * and n == i here. */
                buffer[-1] = 0;
                return (signed char)seq[i + 1];
            }
            i++;
        }
    }
    /* We did not find matching sequence.
     * We possibly read and stored more input in buffer[] by now.
     * n = bytes read. Try to read more until we time out.
     */
got_all:

    if (n <= 1) {
        /* Alt-x is usually returned as ESC x.
         * Report ESC, x is remembered for the next call.
         */
        buffer[-1] = n;
        return 27;
    }

    /* We were doing "buffer[-1] = n; return c;" here, but this results
     * in unknown key sequences being interpreted as ESC + garbage.
     * This was not useful. Pretend there was no key pressed,
     * go and wait for a new keypress:
     */
    buffer[-1] = 0;
    goto start_over;
}

int readit(void) // read (maybe cursor) key from stdin
{
    fflush(stdout);
    return read_key(readbuffer, -1);
}

int get_one_char(void) {
    int c;

    if (!adding2q) {
        // we are not adding to the q.
        // but, we may be reading from a saved q.
        // (checking "ioq" for NULL is wrong, it's not reset to NULL
        // when done - "ioq_start" is reset instead).
        if (ioq_start != NULL) {
            // there is a queue to get chars from.
            // careful with correct sign expansion!
            c = (uint8_t)*ioq++;
            if (c != '\0')
                return c;
            // the end of the q
            free(ioq_start);
            ioq_start = NULL;
            // read from STDIN:
        }
        return readit();
    }
    // we are adding STDIN chars to q.
    c = readit();
    if (lmc_len >= ARRAY_SIZE(last_modifying_cmd) - 2) {
        // last_modifying_cmd[] is too small, can't remember the cmd
        // - drop it
        adding2q = 0;
        lmc_len = 0;
    } else {
        last_modifying_cmd[lmc_len++] = c;
    }
    return c;
}

// Get type of thing to operate on and adjust count
int get_motion_char(void) {
    int c, cnt;

    c = get_one_char();
    if (isdigit(c)) {
        if (c != '0') {
            // get any non-zero motion count
            for (cnt = 0; isdigit(c); c = get_one_char())
                cnt = cnt * 10 + (c - '0');
            cmdcnt = (cmdcnt ?: 1) * cnt;
        } else {
            // ensure standalone '0' works
            cmdcnt = 0;
        }
    }

    return c;
}

// Get input line (uses "status line" area)
char* get_input_line(const char* prompt) {
    // char [MAX_INPUT_LEN]

    int c;
    int i;

    strcpy(get_input_line_buf, prompt);
    last_status_cksum = 0; // force status update
    go_bottom_and_clear_to_eol();
    puts_no_eol(get_input_line_buf); // write out the :, /, or ? prompt

    i = strlen(get_input_line_buf);
    while (i < MAX_INPUT_LEN - 1) {
        c = get_one_char();
        if (c == '\n' || c == '\r' || c == 27)
            break; // this is end of input
        if (c == 8 || c == 127) {
            // user wants to erase prev char
            puts_no_eol("\b \b"); // erase char on screen
            get_input_line_buf[--i] = '\0';
            if (i <= 0) // user backs up before b-o-l, exit
                break;
        } else if (c > 0 && c < 256) { // exclude Unicode
            get_input_line_buf[i] = c;
            get_input_line_buf[++i] = '\0';
            putchar(c);
        }
    }
    refresh(false);
    return get_input_line_buf;
}
