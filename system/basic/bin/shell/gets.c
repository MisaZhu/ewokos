#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <sys/errno.h>

#include <ewoksys/mstr.h>
#include <ewoksys/keydef.h>
#include <poll.h>
#include <utils/telnet_console.h>
#include "shell.h"

void add_history(const char* cmd) {
    if(_history != NULL && strcmp(cmd, _history->cmd->cstr) == 0)
        return;

    old_cmd_t* oc = (old_cmd_t*)malloc(sizeof(old_cmd_t));	
    oc->cmd = str_new(cmd);
    oc->prev = NULL;
    oc->next = _history;
    if(_history != NULL)
        _history->prev = oc;
    else
        _history_tail = oc;
    _history = oc;
}

void free_history(void) {
    old_cmd_t* oc = _history_tail;
    while(oc != NULL) {
        old_cmd_t* prev = oc->prev;
        str_free(oc->cmd);
        free(oc);
        oc = prev;
    }
}

void putch(int c);

static int write_all_retry(int fd, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t off = 0;
    while(off < len) {
        ssize_t wr = write(fd, p + off, len - off);
        if(wr > 0) {
            off += (size_t)wr;
            continue;
        }
        if(errno == EAGAIN || errno == EINTR) {
            proc_usleep(1000);
            continue;
        }
        if(wr == 0 && errno == 0)
            errno = EPIPE;
        return -1;
    }
    return 0;
}

/* Step the cursor back n columns without touching the glyphs. */
static void write_lefts(uint32_t n) {
    char lefts[64];
    while(n > 0) {
        uint32_t m = (n > sizeof(lefts)) ? (uint32_t)sizeof(lefts) : n;
        memset(lefts, CONSOLE_LEFT, m);
        (void)write_all_retry(1, lefts, m);
        n -= m;
    }
}

/*
 * Repaint the line from the cursor to its end, blanking one leftover column
 * when a char was removed, then walk the cursor back to `pos`. That is what
 * makes in-line insertion and deletion visible.
 */
static void redraw_tail(str_t* buf, uint32_t pos, int clear_one) {
    uint32_t tail = buf->len - pos;
    if(tail > 0)
        (void)write_all_retry(1, buf->cstr + pos, tail);
    if(clear_one)
        (void)write_all_retry(1, " ", 1);
    write_lefts(tail + (clear_one ? 1 : 0));
}

/* Blank n screen columns that end at the cursor, leaving the cursor n back. */
static void erase_cols(uint32_t n) {
    char erase[96];
    while(n > 0) {
        uint32_t m = (n > sizeof(erase) / 3) ? (uint32_t)(sizeof(erase) / 3) : n;
        for(uint32_t i = 0; i < m; i++) {
            erase[i * 3]     = CONSOLE_LEFT;
            erase[i * 3 + 1] = ' ';
            erase[i * 3 + 2] = CONSOLE_LEFT;
        }
        (void)write_all_retry(1, erase, m * 3);
        n -= m;
    }
}

/* Move the cursor to end of line, then erase the whole line. */
static void clear_buf(str_t* buf, uint32_t pos, bool show) {
    if(buf->len == 0)
        return;

    if(show) {
        if(pos < buf->len)
            (void)write_all_retry(1, buf->cstr + pos, buf->len - pos);
        erase_cols(buf->len);
    }
    buf->len = 0;
    buf->cstr[0] = 0;
}

/* Insert c at the cursor and advance the cursor; caller repaints the tail. */
static void insert_char(str_t* buf, uint32_t* pos, char c) {
    uint32_t at = (*pos > buf->len) ? buf->len : *pos;

    /* Append first, so str_addc does the capacity growth for us. */
    if(str_addc(buf, c) == NULL)
        return;

    if(at < buf->len - 1) {
        memmove(buf->cstr + at + 1, buf->cstr + at, buf->len - at - 1);
        buf->cstr[at] = c;
        buf->cstr[buf->len] = 0;
    }
    *pos = at + 1;
}

/* Delete the char in front of the cursor and pull the tail back. */
static void erase_char(str_t* buf, uint32_t* pos) {
    if(*pos == 0)
        return;
    memmove(buf->cstr + *pos - 1, buf->cstr + *pos, buf->len - *pos + 1);
    buf->len--;
    (*pos)--;
}

/* ---- TAB filename completion ------------------------------------------ */

#define TAB_WORD_MAX 256   /* longest token we bother completing */

/* Range [start,end) of the whitespace-delimited token the cursor sits in. */
static void token_range(str_t* buf, uint32_t pos, uint32_t* start, uint32_t* end) {
    uint32_t s = pos;
    while(s > 0 && buf->cstr[s-1] != ' ' && buf->cstr[s-1] != '\t')
        s--;
    uint32_t e = pos;
    while(e < buf->len && buf->cstr[e] != ' ' && buf->cstr[e] != '\t')
        e++;
    *start = s;
    *end = e;
}

/*
 * Absolute path of the directory `word` points into; *keep receives how many
 * leading chars of the token form that directory part, so a match can be
 * spliced back without rewriting what the user typed.
 * Returns false for paths the VFS will not resolve (".." components).
 */
static bool token_dir(const char* word, uint32_t wlen,
        char* dir, uint32_t dir_sz, uint32_t* keep) {
    uint32_t k = 0;
    for(uint32_t i = 0; i < wlen; i++) {
        if(word[i] == '/')
            k = i + 1;
    }
    *keep = k;

    const char* rel = word;
    uint32_t rel_len = k;
    if(rel_len >= 2 && rel[0] == '.' && rel[1] == '/') {
        rel += 2;
        rel_len -= 2;
    }
    /* vfsd does not fold "..", so bail out instead of offering a bogus list. */
    for(uint32_t i = 0; i + 1 < rel_len; i++) {
        if(rel[i] == '.' && rel[i+1] == '.')
            return false;
    }

    if(word[0] == '/') {
        snprintf(dir, dir_sz, "%.*s", (int)rel_len, rel);
    }
    else {
        char cwd[FS_FULL_NAME_MAX];
        if(getcwd(cwd, FS_FULL_NAME_MAX-1) == NULL)
            return false;
        if(rel_len == 0)
            snprintf(dir, dir_sz, "%s", cwd);
        else if(strcmp(cwd, "/") == 0)
            snprintf(dir, dir_sz, "/%.*s", (int)rel_len, rel);
        else
            snprintf(dir, dir_sz, "%s/%.*s", cwd, (int)rel_len, rel);
    }

    uint32_t dl = (uint32_t)strlen(dir);
    while(dl > 1 && dir[dl-1] == '/') {
        dir[--dl] = 0;
    }
    return dir[0] != 0;
}

/* Replace buf[start..end) with `repl`, repaint from `start` and leave the
 * cursor right after the replacement. */
static void splice_word(str_t* buf, uint32_t start, uint32_t end,
        const char* repl, uint32_t* pos, bool show) {
    str_t* nb = str_new("");
    if(nb == NULL)
        return;

    uint32_t old_len = buf->len;
    uint32_t rlen = (uint32_t)strlen(repl);

    if(show) {
        /* Park at end of line and wipe from `start`, so the repaint below is
         * correct wherever inside the token the cursor happened to sit. */
        if(*pos < old_len)
            (void)write_all_retry(1, buf->cstr + *pos, old_len - *pos);
        erase_cols(old_len - start);
    }

    /* Rebuild through a temp string so str_cpy does the capacity growth. */
    char save = buf->cstr[start];
    buf->cstr[start] = 0;
    str_add(nb, buf->cstr);
    buf->cstr[start] = save;
    str_add(nb, repl);
    str_add(nb, buf->cstr + end);
    str_cpy(buf, nb->cstr);
    str_free(nb);
    *pos = start + rlen;

    if(show) {
        /* The wipe above stopped at column `start`, so repaint from there: the
         * replacement itself plus any tail that followed the token, then step
         * back to the cursor. Repainting from *pos instead would draw nothing
         * whenever the cursor ends up at end of line - the usual case. */
        if(start < buf->len)
            (void)write_all_retry(1, buf->cstr + start, buf->len - start);
        write_lefts(buf->len - *pos);
    }
}

/* Print the candidates below the prompt, then restore prompt and line. */
static void list_matches(fsinfo_t* kids, uint32_t num,
        const char* prefix, uint32_t plen, str_t* buf, uint32_t pos) {
    /* Finish the current line first, otherwise the text right of the cursor is
     * left stranded on the row above the listing. */
    if(pos < buf->len)
        (void)write_all_retry(1, buf->cstr + pos, buf->len - pos);
    (void)write_all_retry(1, "\n", 1);
    for(uint32_t i = 0; i < num; i++) {
        if(kids[i].name[0] == 0 || strncmp(kids[i].name, prefix, plen) != 0)
            continue;
        (void)write_all_retry(1, kids[i].name, strlen(kids[i].name));
        if(FS_IS_TYPE(kids[i].type, FS_TYPE_DIR))
            (void)write_all_retry(1, "/", 1);
        (void)write_all_retry(1, "  ", 2);
    }
    (void)write_all_retry(1, "\n", 1);
    prompt();
    if(buf->len > 0)
        (void)write_all_retry(1, buf->cstr, buf->len);
    write_lefts(buf->len - pos);
}

/*
 * bash-style filename completion of the token under the cursor: extend it to
 * the longest common prefix of the directory matches, and when a repeated TAB
 * has nothing left to extend, list the candidates below the prompt.
 * Returns the match count; *changed tells whether the line was edited.
 */
static int tab_complete(str_t* buf, uint32_t* pos, bool show,
        bool list_if_stuck, bool* changed) {
    char word[TAB_WORD_MAX];
    /* cwd plus the token's directory part must both fit, hence the extra room. */
    char dir[FS_FULL_NAME_MAX + TAB_WORD_MAX + 2];
    char repl[TAB_WORD_MAX + FS_NODE_NAME_MAX + 2];
    char common[FS_NODE_NAME_MAX];
    uint32_t start, end, keep = 0;

    *changed = false;
    token_range(buf, *pos, &start, &end);

    uint32_t wlen = end - start;
    if(wlen >= sizeof(word))
        return 0;
    memcpy(word, buf->cstr + start, wlen);
    word[wlen] = 0;

    if(!token_dir(word, wlen, dir, sizeof(dir), &keep))
        return 0;

    fsinfo_t info;
    if(vfs_get_by_name(dir, &info) != 0 || !FS_IS_TYPE(info.type, FS_TYPE_DIR))
        return 0;

    uint32_t num = 0;
    fsinfo_t* kids = vfs_kids(&info, &num);
    if(kids == NULL)
        return 0;

    const char* prefix = word + keep;
    uint32_t plen = wlen - keep;
    uint32_t clen = 0;
    uint32_t mtype = 0;
    int matches = 0;
    common[0] = 0;

    for(uint32_t i = 0; i < num; i++) {
        if(kids[i].name[0] == 0 || strncmp(kids[i].name, prefix, plen) != 0)
            continue;
        if(matches == 0) {
            strncpy(common, kids[i].name, sizeof(common)-1);
            common[sizeof(common)-1] = 0;
            clen = (uint32_t)strlen(common);
        }
        else {
            uint32_t j = 0;
            while(j < clen && common[j] == kids[i].name[j])
                j++;
            common[j] = 0;
            clen = j;
        }
        mtype = kids[i].type;
        matches++;
    }

    if(matches == 0) {
        free(kids);
        return 0;
    }

    if(matches == 1) {
        snprintf(repl, sizeof(repl), "%.*s%s", (int)keep, word, common);
        /* A trailing '/' keeps the cursor on the path so the next TAB walks
         * into the directory. Plain files get no trailing space on purpose:
         * the cd builtin takes its argument verbatim instead of splitting
         * words, so "cd foo<TAB> " would fail to resolve. */
        if(FS_IS_TYPE(mtype, FS_TYPE_DIR)) {
            uint32_t rl = (uint32_t)strlen(repl);
            if(rl + 1 < sizeof(repl)) {
                repl[rl] = '/';
                repl[rl+1] = 0;
            }
        }
    }
    else if(clen > plen) {
        snprintf(repl, sizeof(repl), "%.*s%.*s", (int)keep, word, (int)clen, common);
    }
    else {
        /* Nothing left to extend: a second TAB shows what is available. */
        if(list_if_stuck && show)
            list_matches(kids, num, prefix, plen, buf, *pos);
        free(kids);
        return matches;
    }

    if(strcmp(repl, word) != 0) {
        splice_word(buf, start, end, repl, pos, show);
        *changed = true;
    }
    free(kids);
    return matches;
}

static telnet_console_t _telnet_console;

int32_t cmd_gets(int fd, str_t* buf) {
    str_reset(buf);	
    old_cmd_t* head = NULL;
    old_cmd_t* tail = NULL;
    bool first_up = true;
    bool telnet = (fd == 0) && telnet_console_is_active();
    bool echo = true;
    uint32_t pos = 0;   /* cursor offset inside buf, always 0..buf->len */
    int esc = 0;        /* 0: normal, 1: got ESC, 2: got "ESC [", 3: got "ESC O" */
    bool tab_again = false;  /* a repeated TAB lists the candidates */

    while(1) {
        char c, old_c;
        errno = 0;
        int i = telnet ? telnet_console_read(fd, &_telnet_console, &c) : read(fd, &c, 1);
        if(i == 0) {
            /*
             * Keep local console stdin tolerant of transient empty reads, but a
             * telnet-backed stdin must treat read(2)==0 as EOF so the remote shell
             * exits cleanly when the client disconnects.
             * For local consoles, check poll events: if the device reports
             * CLOSE/NVAL/ERR (or is gone), treat as EOF so the shell exits
             * when e.g. the xterm window is closed.
             */
            if(fd == 0 && !_script_mode && !telnet) {
                uint32_t ev = vfs_get_poll_events(fd);
                if(ev == 0 || (ev & (VFS_EVT_CLOSE | VFS_EVT_NVAL | VFS_EVT_ERR)) != 0)
                    return -1;
                proc_usleep(10000);
                continue;
            }
            return -1;
        }
        if(i < 0) {
            if(errno == EAGAIN || errno == EINTR || errno == 0) {
                proc_usleep(10000);
                continue;
            }
            return -1;
        }
        
        if(telnet) {
            c = telnet_console_parse(&_telnet_console, (uint8_t)c);
            if(c == 0) {
                continue;
            }
        }
        if(c == 0 || i < 0) {
            proc_usleep(10000);
            continue;
        }

        /*
         * xterm and consoled deliver the arrow keys as EwokOS single-byte key
         * codes, but a serial /dev/tty0 (and telnet/ssh) sends the ANSI forms
         * ESC [ A and ESC O A. Fold both into the same codes so the editing
         * below behaves identically on every console.
         */
        if(esc != 0) {
            if(esc == 1) {
                if(c == '[' || c == 'O') {
                    esc = (c == '[') ? 2 : 3;
                    continue;
                }
                esc = 0;   /* lone ESC, or ESC x: treat x as an ordinary key */
            }
            else {
                if(esc == 2 && ((c >= '0' && c <= '9') || c == ';' || c == '?'))
                    continue;   /* CSI parameter byte: stay inside the sequence */
                esc = 0;
                if(c == 'A')
                    c = KEY_UP;
                else if(c == 'B')
                    c = KEY_DOWN;
                else if(c == 'C')
                    c = KEY_RIGHT;
                else if(c == 'D')
                    c = KEY_LEFT;
                else
                    continue;   /* unknown escape: drop it */
            }
        }
        else if(c == KEY_ESC) {
            esc = 1;
            continue;
        }

        bool show = echo && !_script_mode;
        bool tab_now = (c == KEY_TAB && show);
        if(!tab_now)
            tab_again = false;

        if (tab_now) {
            bool changed = false;
            int n = tab_complete(buf, &pos, show, tab_again, &changed);
            tab_again = (n > 1 && !changed);
        }
        else if (c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
            if (pos > 0) {
                //delete the char in front of the cursor
                const char left = CONSOLE_LEFT;
                erase_char(buf, &pos);
                if(show) {
                    (void)write_all_retry(1, &left, 1);
                    redraw_tail(buf, pos, 1);
                }
            }
        }
        else if (c == KEY_LEFT) { //move cursor left
            if (pos > 0) {
                pos--;
                if(show)
                    write_lefts(1);
            }
        }
        else if (c == KEY_RIGHT) { //move cursor right
            if (pos < buf->len) {
                if(show)
                    (void)write_all_retry(1, buf->cstr + pos, 1);
                pos++;
            }
        }
        else if (c == KEY_UP) { //prev command
            if(first_up) {
                head = _history;
                first_up = false;
            }
            else if(head != NULL) {
                head = head->next;
            }

            if(head != NULL) {
                tail = head;
                clear_buf(buf, pos, show);
                str_cpy(buf, head->cmd->cstr);
                if(buf->len > 0 && show)
                    (void)write_all_retry(1, buf->cstr, buf->len);
                pos = buf->len;
            }
        }
        else if (c == KEY_DOWN) { //next command
            if(tail != NULL)
                tail = tail->prev;
            clear_buf(buf, pos, show);
            pos = 0;

            if(tail != NULL) {
                head = tail;
                str_cpy(buf, tail->cmd->cstr);
                if(buf->len > 0 && show)
                    (void)write_all_retry(1, buf->cstr, buf->len);
                pos = buf->len;
            }
            else {
                head = _history;
                first_up = true;
            }
        }
        else {
            if(c == '\r') {
                old_c = c;
                c = '\n';
            }
            else  {
                old_c = 0;
                if(c == '\n' && old_c == '\r')
                    continue;
            }

            if(buf->len == 0 && (c == '@' || c == '#'))
                echo = false;
            show = echo && !_script_mode;

            if(c == '\n') {
                /* Break the line at its end, not where the cursor sits. */
                if(show && pos < buf->len)
                    (void)write_all_retry(1, buf->cstr + pos, buf->len - pos);
                if(show)
                    putch(c);
                break;
            }

            if(show)
                putch(c);
            if(c > 27) {
                insert_char(buf, &pos, c);
                if(show && pos < buf->len)
                    redraw_tail(buf, pos, 0);
            }
        }
    }
    str_addc(buf, 0);
    return 0;
}
