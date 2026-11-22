/* vim_text.c — text buffer, line navigation, undo and file I/O (split from vi.c) */
#include "vim.h"
#include "vim_syntax.h"

//----- Text Movement Routines ---------------------------------
char* begin_line(char* p) // return pointer to first char cur line
{
    if (p > text) {
        p = memrchr(text, '\n', p - text);
        if (!p)
            return text;
        return p + 1;
    }
    return p;
}

char* end_line(char* p) // return pointer to NL of cur line
{
    if (p < end - 1) {
        p = memchr(p, '\n', end - p - 1);
        if (!p)
            return end - 1;
    }
    return p;
}

char* dollar_line(char* p) // return pointer to just before NL line
{
    p = end_line(p);
    // Try to stay off of the Newline
    if (*p == '\n' && (p - begin_line(p)) > 0)
        p--;
    return p;
}

char* prev_line(char* p) // return pointer first char prev line
{
    p = begin_line(p); // goto beginning of cur line
    if (p > text && p[-1] == '\n')
        p--;           // step to prev line
    p = begin_line(p); // goto beginning of prev line
    return p;
}

char* next_line(char* p) // return pointer first char next line
{
    p = end_line(p);
    if (p < end - 1 && *p == '\n')
        p++; // step to next line
    return p;
}

//----- Text Information Routines ------------------------------
char* end_screen(void) {
    char* q;
    int cnt;

    // find new bottom line
    q = screenbegin;
    for (cnt = 0; cnt < rows - 2; cnt++)
        q = next_line(q);
    q = end_line(q);
    return q;
}

// count line from start to stop
int count_lines(char* start, char* stop) {
    char* q;
    int cnt;

    if (stop < start) { // start and stop are backwards- reverse them
        q = start;
        start = stop;
        stop = q;
    }
    cnt = 0;
    stop = end_line(stop);
    while (start <= stop && start <= end - 1) {
        start = end_line(start);
        if (*start == '\n')
            cnt++;
        start++;
    }
    return cnt;
}

char* find_line(int li) // find beginning of line #li
{
    char* q;

    for (q = text; li > 1; li--) {
        q = next_line(q);
    }
    return q;
}

int next_tabstop(int col) { return col + ((tabstop - 1) - (col % tabstop)); }

int prev_tabstop(int col) { return col - ((col % tabstop) ?: tabstop); }

int next_column(char c, int co) {
    if (c == '\t')
        co = next_tabstop(co);
    else if ((uint8_t)c < ' ' || c == 0x7f)
        co++; // display as ^X, use 2 columns
    return co + 1;
}

int get_column(char* p) {
    const char* r;
    int co = 0;

    for (r = begin_line(p); r < p; r++)
        co = next_column(*r, co);
    return co;
}

//----- Block insert/delete, undo ops --------------------------
char* text_yank(char* p, char* q, int dest, int buftype) {
    char* oldreg = reg[dest];
    int cnt = q - p;
    if (cnt < 0) { // they are backwards- reverse them
        p = q;
        cnt = -cnt;
    }
    // Don't free register yet.  This prevents the memory allocator
    // from reusing the free block so we can detect if it's changed.
    reg[dest] = strndup(p, cnt + 1);
    regtype[dest] = buftype;
    free(oldreg);
    return p;
}

char what_reg(void) {
    char c;

    c = 'D'; // default to D-reg
    if (YDreg <= 25)
        c = 'a' + (char)YDreg;
    if (YDreg == 26)
        c = 'D';
    if (YDreg == 27)
        c = 'U';
    return c;
}

void check_context(char cmd) {
    // Certain movement commands update the context.
    if (strchr(":%{}'GHLMz/?Nn", cmd) != NULL) {
        mark[27] = mark[26]; // move cur to prev
        mark[26] = dot;      // move local to cur
    }
}

char* swap_context(char* p) // goto new context for '' command make this the current context
{
    char* tmp;

    // the current context is in mark[26]
    // the previous context is in mark[27]
    // only swap context if other context is valid
    if (text <= mark[27] && mark[27] <= end - 1) {
        tmp = mark[27];
        mark[27] = p;
        mark[26] = p = tmp;
    }
    return p;
}

void yank_status(const char* op, const char* p, int cnt) {
    int lines, chars;

    lines = chars = 0;
    while (*p) {
        ++chars;
        if (*p++ == '\n')
            ++lines;
    }
    status_line("%s %d lines (%d chars) from [%c]", op, lines * cnt, chars * cnt, what_reg());
}

// open a hole in text[]
// might reallocate text[]! use p += text_hole_make(p, ...),
// and be careful to not use pointers into potentially freed text[]!
intptr_t text_hole_make(char* p, int size) // at "p", make a 'size' byte hole
{
    intptr_t bias = 0;

    if (size <= 0)
        return bias;
    end += size; // adjust the new END
    if (end >= (text + text_size)) {
        char* new_text;
        text_size += end - (text + text_size) + 10240;
        new_text = realloc(text, text_size);
        bias = (new_text - text);
        screenbegin += bias;
        dot += bias;
        end += bias;
        p += bias;
        {
            int i;
            for (i = 0; i < ARRAY_SIZE(mark); i++)
                if (mark[i])
                    mark[i] += bias;
            if (vi_visual_anchor != NULL) // keep the selection anchor in text[]
                vi_visual_anchor += bias;
        }
        text = new_text;
    }
    memmove(p + size, p, end - size - p);
    memset(p, ' ', size); // clear new hole
    return bias;
}

void undo_push(char* src, uint32_t length, int u_type);

// close a hole in text[] - delete "p" through "q", inclusive
// "undo" value indicates if this operation should be undo-able
char* text_hole_delete(char* p, char* q, int undo) {
    char *src, *dest;
    int cnt, hole_size;

    // move forwards, from beginning
    // assume p <= q
    src = q + 1;
    dest = p;
    if (q < p) { // they are backward- swap them
        src = p + 1;
        dest = q;
    }
    hole_size = q - p + 1;
    cnt = end - src;
    switch (undo) {
    case NO_UNDO:
        break;
    case ALLOW_UNDO:
        undo_push(p, hole_size, UNDO_DEL);
        break;
    case ALLOW_UNDO_CHAIN:
        undo_push(p, hole_size, UNDO_DEL_CHAIN);
        break;
    case ALLOW_UNDO_QUEUED:
        undo_push(p, hole_size, UNDO_DEL_QUEUED);
        break;
    }
    modified_count--;
    if (src < text || src > end)
        goto thd0;
    if (dest < text || dest >= end)
        goto thd0;
    modified_count++;
    if (src >= end)
        goto thd_atend; // just delete the end of the buffer
    memmove(dest, src, cnt);
thd_atend:
    end = end - hole_size; // adjust the new END
    if (dest >= end)
        dest = end - 1; // make sure dest in below end-1
    if (end <= text)
        dest = end = text; // keep pointers valid
thd0:
    return dest;
}

// Flush any queued objects to the undo stack
void undo_queue_commit(void) {
    // Pushes the queue object onto the undo stack
    if (undo_q > 0) {
        // Deleted character undo events grow from the end
        undo_push(undo_queue + VI_UNDO_QUEUE_MAX - undo_q, undo_q,
                  (undo_queue_state | UNDO_USE_SPOS));
        undo_queue_state = UNDO_EMPTY;
        undo_q = 0;
    }
}

void undo_push(char* src, uint32_t length, int u_type) {
    struct undo_object* undo_entry;
    int use_spos = u_type & UNDO_USE_SPOS;

    // "u_type" values
    // UNDO_INS: insertion, undo will remove from buffer
    // UNDO_DEL: deleted text, undo will restore to buffer
    // UNDO_{INS,DEL}_CHAIN: Same as above but also calls undo_pop() when complete
    // The CHAIN operations are for handling multiple operations that the user
    // performs with a single action, i.e. REPLACE mode or find-and-replace commands
    // UNDO_{INS,DEL}_QUEUED: If queuing feature is enabled, allow use of the queue
    // for the INS/DEL operation.
    // UNDO_{INS,DEL} ORed with UNDO_USE_SPOS: commit the undo queue

    // This undo queuing functionality groups multiple character typing or backspaces
    // into a single large undo object. This greatly reduces calls to malloc() for
    // single-character operations while typing and has the side benefit of letting
    // an undo operation remove chunks of text rather than a single character.
    switch (u_type) {
    case UNDO_EMPTY: // Just in case this ever happens...
        return;
    case UNDO_DEL_QUEUED:
        if (length != 1)
            return; // Only queue single characters
        switch (undo_queue_state) {
        case UNDO_EMPTY:
            undo_queue_state = UNDO_DEL;
        case UNDO_DEL:
            undo_queue_spos = src;
            undo_q++;
            undo_queue[VI_UNDO_QUEUE_MAX - undo_q] = *src;
            // If queue is full, dump it into an object
            if (undo_q == VI_UNDO_QUEUE_MAX)
                undo_queue_commit();
            return;
        case UNDO_INS:
            // Switch from storing inserted text to deleted text
            undo_queue_commit();
            undo_push(src, length, UNDO_DEL_QUEUED);
            return;
        }
        break;
    case UNDO_INS_QUEUED:
        if (length < 1)
            return;
        switch (undo_queue_state) {
        case UNDO_EMPTY:
            undo_queue_state = UNDO_INS;
            undo_queue_spos = src;
        case UNDO_INS:
            while (length--) {
                undo_q++; // Don't need to save any data for insertions
                if (undo_q == VI_UNDO_QUEUE_MAX)
                    undo_queue_commit();
            }
            return;
        case UNDO_DEL:
            // Switch from storing deleted text to inserted text
            undo_queue_commit();
            undo_push(src, length, UNDO_INS_QUEUED);
            return;
        }
        break;
    }
    u_type &= ~UNDO_USE_SPOS;

    // Allocate a new undo object
    if (u_type == UNDO_DEL || u_type == UNDO_DEL_CHAIN) {
        // For UNDO_DEL objects, save deleted text
        if ((text + length) == end)
            length--;
        // If this deletion empties text[], strip the newline. When the buffer becomes
        // zero-length, a newline is added back, which requires this to compensate.
        undo_entry = zalloc(offsetof(struct undo_object, undo_text) + length);
        memcpy(undo_entry->undo_text, src, length);
    } else {
        undo_entry = zalloc(sizeof(*undo_entry));
    }
    undo_entry->length = length;
    if (use_spos) {
        undo_entry->start = undo_queue_spos - text; // use start position from queue
    } else {
        undo_entry->start = src - text; // use offset from start of text buffer
    }
    undo_entry->u_type = u_type;

    // Push it on undo stack
    undo_entry->prev = undo_stack_tail;
    undo_stack_tail = undo_entry;
    modified_count++;
}

void flush_undo_data(void) {
    struct undo_object* undo_entry;

    while (undo_stack_tail) {
        undo_entry = undo_stack_tail;
        undo_stack_tail = undo_entry->prev;
        free(undo_entry);
    }
}

void undo_push_insert(char* p, int len, int undo) {
    switch (undo) {
    case ALLOW_UNDO:
        undo_push(p, len, UNDO_INS);
        break;
    case ALLOW_UNDO_CHAIN:
        undo_push(p, len, UNDO_INS_CHAIN);
        break;
    case ALLOW_UNDO_QUEUED:
        undo_push(p, len, UNDO_INS_QUEUED);
        break;
    }
}

intptr_t string_insert(char* p, const char* s, int undo) // insert the string at 'p'
{
    intptr_t bias;
    int i;

    i = strlen(s);
    undo_push_insert(p, i, undo);
    bias = text_hole_make(p, i);
    p += bias;
    memcpy(p, s, i);
    return bias;
}

// Undo the last operation
void undo_pop(void) {
    int repeat;
    char *u_start, *u_end;
    struct undo_object* undo_entry;

    // Commit pending undo queue before popping (should be unnecessary)
    undo_queue_commit();

    undo_entry = undo_stack_tail;
    // Check for an empty undo stack
    if (!undo_entry) {
        status_line("Already at oldest change");
        return;
    }

    switch (undo_entry->u_type) {
    case UNDO_DEL:
    case UNDO_DEL_CHAIN:
        // make hole and put in text that was deleted; deallocate text
        u_start = text + undo_entry->start;
        text_hole_make(u_start, undo_entry->length);
        memcpy(u_start, undo_entry->undo_text, undo_entry->length);
        status_line("Undo [%d] %s %d chars at position %d", modified_count, "restored",
                    undo_entry->length, undo_entry->start);
        break;
    case UNDO_INS:
    case UNDO_INS_CHAIN:
        // delete what was inserted
        u_start = undo_entry->start + text;
        u_end = u_start - 1 + undo_entry->length;
        text_hole_delete(u_start, u_end, NO_UNDO);
        status_line("Undo [%d] %s %d chars at position %d", modified_count, "deleted",
                    undo_entry->length, undo_entry->start);
        break;
    }
    repeat = 0;
    switch (undo_entry->u_type) {
    // If this is the end of a chain, lower modification count and refresh display
    case UNDO_DEL:
    case UNDO_INS:
        dot = (text + undo_entry->start);
        refresh(false);
        break;
    case UNDO_DEL_CHAIN:
    case UNDO_INS_CHAIN:
        repeat = 1;
        break;
    }
    // Deallocate the undo object we just processed
    undo_stack_tail = undo_entry->prev;
    free(undo_entry);
    modified_count--;
    // For chained operations, continue popping all the way down the chain.
    if (repeat) {
        undo_pop(); // Follow the undo chain if one exists
    }
}

//----- Dot Movement Routines ----------------------------------
void dot_left(void) {
    undo_queue_commit();
    if (dot > text && dot[-1] != '\n')
        dot--;
}

void dot_right(void) {
    undo_queue_commit();
    if (dot < end - 1 && *dot != '\n')
        dot++;
}

void dot_begin(void) {
    undo_queue_commit();
    dot = begin_line(dot); // return pointer to first char cur line
}

void dot_end(void) {
    undo_queue_commit();
    dot = end_line(dot); // return pointer to last char cur line
}

char* move_to_col(char* p, int l) {
    int co;

    p = begin_line(p);
    co = 0;
    do {
        if (*p == '\n') // vda || *p == '\0')
            break;
        co = next_column(*p, co);
    } while (co <= l && p++ < end);
    return p;
}

void dot_next(void) {
    undo_queue_commit();
    dot = next_line(dot);
}

void dot_prev(void) {
    undo_queue_commit();
    dot = prev_line(dot);
}

void dot_skip_over_ws(void) {
    // skip WS
    while (isspace(*dot) && *dot != '\n' && dot < end - 1)
        dot++;
}

void dot_to_char(int cmd) {
    char* q = dot;
    int dir = islower(cmd) ? FORWARD : BACK;

    if (last_search_char == 0)
        return;

    do {
        do {
            q += dir;
            if ((dir == FORWARD ? q > end - 1 : q < text) || *q == '\n') {
                indicate_error();
                return;
            }
        } while (*q != last_search_char);
    } while (--cmdcnt > 0);

    dot = q;

    // place cursor before/after char as required
    if (cmd == 't')
        dot_left();
    else if (cmd == 'T')
        dot_right();
}

void dot_scroll(int cnt, int dir) {
    char* q;

    undo_queue_commit();
    for (; cnt > 0; cnt--) {
        if (dir < 0) {
            // scroll Backwards
            // ctrl-Y scroll up one line
            screenbegin = prev_line(screenbegin);
        } else {
            // scroll Forwards
            // ctrl-E scroll down one line
            screenbegin = next_line(screenbegin);
        }
    }
    // make sure "dot" stays on the screen so we dont scroll off
    if (dot < screenbegin)
        dot = screenbegin;
    q = end_screen(); // find new bottom line
    if (dot > q)
        dot = begin_line(q); // is dot is below bottom line?
    dot_skip_over_ws();
}

char* bound_dot(char* p) // make sure  text[0] <= P < "end"
{
    if (p >= end && end > text) {
        p = end - 1;
        indicate_error();
    }
    if (p < text) {
        p = text;
        indicate_error();
    }
    return p;
}

void start_new_cmd_q(char c) {
    // get buffer for new cmd
    dotcnt = cmdcnt ?: 1;
    last_modifying_cmd[0] = c;
    lmc_len = 1;
    adding2q = 1;
}
void end_cmd_q(void) {
    YDreg = 26; // go back to default Yank/Delete reg
    adding2q = 0;
}

// copy text into register, then delete text.
//
char* yank_delete(char* start, char* stop, int buftype, int yf, int undo) {
    char* p;

    // make sure start <= stop
    if (start > stop) {
        // they are backwards, reverse them
        p = start;
        start = stop;
        stop = p;
    }
    if (buftype == PARTIAL && *start == '\n')
        return start;
    p = start;
    text_yank(start, stop, YDreg, buftype);
    if (yf == YANKDEL) {
        p = text_hole_delete(start, stop, undo);
    } // delete lines
    return p;
}

// might reallocate text[]!
int file_insert(const char* fn, char* p, int initial) {
    if (fn == NULL)
        return -1;
    int cnt = -1;
    int fd;
    int size;
    struct stat statbuf;

    if (p < text)
        p = text;
    if (p > end)
        p = end;

    if (stat(fn, &statbuf) < 0) {
        if (!initial)
            status_line_bold_errno(fn);
        return cnt;
    }

    size = (statbuf.st_size < 0x7fffffff ? statbuf.st_size : 0x7fffffff);
    p += text_hole_make(p, size);
    fd = open(fn, O_RDONLY);
    if(fd < 0) {
        if (!initial)
            status_line_bold_errno(fn);
        return cnt;
    }
    cnt = read(fd, p, size);
    if (cnt < 0) {
        status_line_bold_errno(fn);
        p = text_hole_delete(p, p + size - 1, NO_UNDO); // un-do buffer insert
    } else if (cnt < size) {
        // There was a partial read, shrink unused space
        p = text_hole_delete(p + cnt, p + size - 1, NO_UNDO);
        status_line_bold("can't read '%s'", fn);
    } else {
        undo_push_insert(p, size, ALLOW_UNDO);
    }
fi:
    close(fd);

    return cnt;
}

// find matching char of pair  ()  []  {}
// will crash if c is not one of these
char* find_pair(char* p, const char c) {
    const char* braces = "()[]{}";
    char match;
    int dir, level;

    dir = strchr(braces, c) - braces;
    dir ^= 1;
    match = braces[dir];
    dir = ((dir & 1) << 1) - 1; // 1 for ([{, -1 for )\}

    // look for match, count levels of pairs  (( ))
    level = 1;
    for (;;) {
        p += dir;
        if (p < text || p >= end)
            return NULL;
        if (*p == c)
            level++; // increase pair levels
        if (*p == match) {
            level--; // reduce pair level
            if (level == 0)
                return p; // found matching pair
        }
    }
}

// show the matching char of a pair,  ()  []  {}
void showmatching(char* p) {
    char *q, *save_dot;

    // we found half of a pair
    q = find_pair(p, *p); // get loc of matching char
    if (q == NULL) {
        indicate_error(); // no matching char
    } else {
        // "q" now points to matching pair
        save_dot = dot; // remember where we are
        dot = q;        // go to new loc
        refresh(false); // let the user see it
        usleep(1000); // give user some time
        dot = save_dot; // go back to old loc
        refresh(false);
    }
}

// might reallocate text[]! use p += stupid_insert(p, ...),
// and be careful to not use pointers into potentially freed text[]!
intptr_t stupid_insert(char* p, char c) // stupidly insert the char c at 'p'
{
    intptr_t bias;
    bias = text_hole_make(p, 1);
    p += bias;
    *p = c;
    return bias;
}

// find number of characters in indent, p must be at beginning of line
size_t indent_len(char* p) {
    char* r = p;

    while (r < (end - 1) && isblank(*r))
        r++;
    return r - p;
}

char* char_insert(char* p, char c, int undo) // insert the char c at 'p'
{
    size_t len;
    int col, ntab, nspc;
    char* bol = begin_line(p);

    if (c == 22) {                  // Is this an ctrl-V?
        p += stupid_insert(p, '^'); // use ^ to indicate literal next
        refresh(false);             // show the ^
        c = get_one_char();
        *p = c;
        undo_push_insert(p, 1, undo);
        p++;
    } else if (c == 27) { // Is this an ESC?
        cmd_mode = 0;
        undo_queue_commit();
        cmdcnt = 0;
        end_cmd_q();           // stop adding to q
        last_status_cksum = 0; // force status update
        if ((dot > text) && (p[-1] != '\n')) {
            p--;
        }
        if (autoindent) {
            len = indent_len(bol);
            if (len && get_column(bol + len) == indentcol && bol[len] == '\n') {
                // remove autoindent from otherwise empty line
                text_hole_delete(bol, bol + len - 1, undo);
                p = bol;
            }
        }
    } else if (c == 4) { // ctrl-D reduces indentation
        char* r = bol + indent_len(bol);
        int prev = prev_tabstop(get_column(r));
        while (r > bol && get_column(r) > prev) {
            if (p > bol)
                p--;
            r--;
            r = text_hole_delete(r, r, ALLOW_UNDO_QUEUED);
        }

        if (autoindent && indentcol && r == end_line(p)) {
            // record changed size of autoindent
            indentcol = get_column(p);
            return p;
        }
    } else if (c == '\t' && expandtab) { // expand tab
        col = get_column(p);
        col = next_tabstop(col) - col + 1;
        while (col--) {
            undo_push_insert(p, 1, undo);
            p += 1 + stupid_insert(p, ' ');
        }
    } else if (c == 8 || c == 127) { // Is this a BS
        if (p > text) {
            p--;
            p = text_hole_delete(p, p, ALLOW_UNDO_QUEUED); // shrink buffer 1 char
        }
    } else {
        // insert a char into text[]
        if (c == 13)
            c = '\n'; // translate \r to \n
        if (c == '\n')
            undo_queue_commit();
        p += 1 + stupid_insert(p, c); // insert the char
        if (showmatch && strchr(")]}", c) != NULL) {
            showmatching(p - 1);
        }
        if (autoindent && c == '\n') { // auto indent the new line
            // use indent of current/previous line
            bol = indentcol < 0 ? p : prev_line(p);
            len = indent_len(bol);
            col = get_column(bol + len);

            if (len && col == indentcol) {
                // previous line was empty except for autoindent
                // move the indent to the current line
                memmove(bol + 1, bol, len);
                *bol = '\n';
                return p;
            }

            if (indentcol < 0)
                p--; // open above, indent before newly inserted NL

            if (len) {
                indentcol = col;
                if (expandtab) {
                    ntab = 0;
                    nspc = col;
                } else {
                    ntab = col / tabstop;
                    nspc = col % tabstop;
                }
                p += text_hole_make(p, ntab + nspc);
                undo_push_insert(p, ntab + nspc, undo);
                memset(p, '\t', ntab);
                p += ntab;
                memset(p, ' ', nspc);
                return p + nspc;
            }
        }
    }
    indentcol = 0;
    return p;
}

void init_filename(char* fn) {
    if (current_filename == NULL) {
        current_filename = strdup(fn);
    }
}

void update_filename(char* fn) {
    if (fn == NULL)
        return;
    syntax_set_file(fn); // pick .json/.rd/.conf/.js highlighting for this file
    if (fn != current_filename) {
        free(current_filename);
        current_filename = strdup(fn);
    }
}

// read text from file or create an empty buf
// will also update current_filename
int init_text_buffer(char* fn) {
    int rc;

    // allocate/reallocate text buffer
    if (text)
        free(text);
    text_size = 10240;
    screenbegin = dot = end = text = zalloc(text_size);

    update_filename(fn);
    rc = file_insert(fn, text, 1);
    if (rc < 0) {
        // file doesnt exist. Start empty buf with dummy line
        char_insert(text, '\n', NO_UNDO);
    }

    flush_undo_data();
    modified_count = 0;
    last_modified_count = -1;
    // init the marks
    memset(mark, 0, sizeof(mark));
    return rc;
}

int file_write(char* fn, char* first, char* last) {
    int fd;
    int cnt, charcnt;

    if (fn == 0) {
        status_line_bold("No current filename");
        return -2;
    }
    // By popular request we do not open file with O_TRUNC,
    // but instead ftruncate() it _after_ successful write.
    // Might reduce amount of data lost on power fail etc.
    //fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC);
    fd = open(fn, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
        return -1;
    cnt = last - first + 1;
    charcnt = write(fd, first, cnt);
    if (charcnt == cnt) {
        // good write
        // modified_count = false;
    } else {
        charcnt = 0;
    }
    close(fd);
    return charcnt;
}

int mycmp(const char* s1, const char* s2, int len) {
    if (ignorecase) {
        return strncasecmp(s1, s2, len);
    }
    return strncmp(s1, s2, len);
}

char* char_search(char* p, const char* pat, int dir_and_range) {
    char *start, *stop;
    int len;
    int range;

    len = strlen(pat);
    range = (dir_and_range & 1);
    if (dir_and_range > 0) { // FORWARD?
        stop = end - 1;      // assume range is p..end-1
        if (range == LIMITED)
            stop = next_line(p); // range is to next line
        for (start = p; start < stop; start++) {
            if (mycmp(start, pat, len) == 0) {
                return start;
            }
        }
    } else {         // BACK
        stop = text; // assume range is text..p
        if (range == LIMITED)
            stop = prev_line(p); // range is to prev line
        for (start = p - len; start >= stop; start--) {
            if (mycmp(start, pat, len) == 0) {
                return start;
            }
        }
    }
    // pattern not found
    return NULL;
}
