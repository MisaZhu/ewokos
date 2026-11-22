/* vim_cmd.c — ex/colon commands and helpers (split from vi.c) */
#include "vim.h"

//----- The Colon commands -------------------------------------
// Evaluate colon address expression.  Returns a pointer to the
// next character or NULL on error.  If 'result' contains a valid
// address 'valid' is true.

char* get_one_address(char* p, int* result, int* valid) {
    int num, sign, addr, got_addr;
    char *q, c;
    int dir;

    got_addr = false;
    addr = count_lines(text, dot); // default to current line
    sign = 0;
    for (;;) {
        if (isblank(*p)) {
            if (got_addr) {
                addr += sign;
                sign = 0;
            }
            p++;
        } else if (!got_addr && *p == '.') { // the current line
            p++;
            // addr = count_lines(text, dot);
            got_addr = true;
        } else if (!got_addr && *p == '$') { // the last line in file
            p++;
            addr = count_lines(text, end - 1);
            got_addr = true;
        } else if (!got_addr && *p == '\'') { // is this a mark addr
            p++;
            c = tolower(*p);
            p++;
            q = NULL;
            if (c >= 'a' && c <= 'z') {
                // we have a mark
                c = c - 'a';
                q = mark[(uint8_t)c];
            }
            if (q == NULL) { // is mark valid
                status_line_bold("Mark not set");
                return NULL;
            }
            addr = count_lines(text, q);
            got_addr = true;
        } else if (!got_addr && (*p == '/' || *p == '?')) { // a search pattern
            c = *p;
            q = strchrnul(p + 1, c);
            if (p + 1 != q) {
                // save copy of new pattern
                free(last_search_pattern);
                last_search_pattern = strndup(p, q - p);
            }
            p = q;
            if (*p == c)
                p++;
            if (c == '/') {
                q = next_line(dot);
                dir = (FORWARD << 1) | FULL;
            } else {
                q = begin_line(dot);
                dir = ((uint32_t)BACK << 1) | FULL;
            }
            q = char_search(q, last_search_pattern + 1, dir);
            if (q == NULL) {
                // no match, continue from other end of file
                q = char_search(dir > 0 ? text : end - 1, last_search_pattern + 1, dir);
                if (q == NULL) {
                    status_line_bold("Pattern not found");
                    return NULL;
                }
            }
            addr = count_lines(text, q);
            got_addr = true;
        } else if (isdigit(*p)) {
            num = 0;
            while (isdigit(*p))
                num = num * 10 + *p++ - '0';
            if (!got_addr) { // specific line number
                addr = num;
                got_addr = true;
            } else { // offset from current addr
                addr += sign >= 0 ? num : -num;
            }
            sign = 0;
        } else if (*p == '-' || *p == '+') {
            if (!got_addr) { // default address is dot
                // addr = count_lines(text, dot);
                got_addr = true;
            } else {
                addr += sign;
            }
            sign = *p++ == '-' ? -1 : 1;
        } else {
            addr += sign; // consume unused trailing sign
            break;
        }
    }
    *result = addr;
    *valid = got_addr;
    return p;
}

#define GET_ADDRESS 0
#define GET_SEPARATOR 1

// Read line addresses for a colon command.  The user can enter as
// many as they like but only the last two will be used.
char* get_address(char* p, int* b, int* e, uint32_t* got) {
    int state = GET_ADDRESS;
    int valid;
    int addr;
    char* save_dot = dot;

    //----- get the address' i.e., 1,3   'a,'b  -----
    for (;;) {
        if (isblank(*p)) {
            p++;
        } else if (state == GET_ADDRESS && *p == '%') { // alias for 1,$
            p++;
            *b = 1;
            *e = count_lines(text, end - 1);
            *got = 3;
            state = GET_SEPARATOR;
        } else if (state == GET_ADDRESS) {
            valid = false;
            p = get_one_address(p, &addr, &valid);
            // Quit on error or if the address is invalid and isn't of
            // the form ',$' or '1,' (in which case it defaults to dot).
            if (p == NULL || !(valid || *p == ',' || *p == ';' || *got & 1))
                break;
            *b = *e;
            *e = addr;
            *got = (*got << 1) | 1;
            state = GET_SEPARATOR;
        } else if (state == GET_SEPARATOR && (*p == ',' || *p == ';')) {
            if (*p == ';')
                dot = find_line(*e);
            p++;
            state = GET_ADDRESS;
        } else {
            break;
        }
    }
    dot = save_dot;
    return p;
}

void setops(char* args, int flg_no) {
    char* eq;
    int index;

    eq = strchr(args, '=');
    if (eq)
        *eq = '\0';
    index = index_in_strings(OPTS_STR, args + flg_no);
    if (eq)
        *eq = '=';
    if (index < 0) {
    bad:
        status_line_bold("bad option: %s", args);
        return;
    }

    index = 1 << (index >> 1); // convert to VI_bit

    if (index & VI_TABSTOP) {
        int t;
        if (!eq || flg_no) // no "=NNN" or it is "notabstop"?
            goto bad;
        errno = 0;
        t = strtoul(eq + 1, NULL, 10);
        if (errno == ERANGE)
            t = -1;
        if (t <= 0 || t > MAX_TABSTOP)
            goto bad;
        tabstop = t;
        return;
    }
    if (eq)
        goto bad; // boolean option has "="?
    if (flg_no) {
        vi_setops &= ~index;
    } else {
        vi_setops |= index;
    }
}

char* skip_whitespace(const char* s) {
    /* In POSIX/C locale (the only locale we care about: do we REALLY want
     * to allow Unicode whitespace in, say, .conf files? nuts!)
     * isspace is only these chars: "\t\n\v\f\r" and space.
     * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
     * Use that.
     */
    while (*s == ' ' || (uint8_t)(*s - 9) <= (13 - 9))
        s++;

    return (char*)s;
}

char* skip_non_whitespace(const char* s) {
    while (*s != '\0' && *s != ' ' && (uint8_t)(*s - 9) > (13 - 9))
        s++;

    return (char*)s;
}

#define strchr_backslash(s, c) strchr(s, c)

// buf must be no longer than MAX_INPUT_LEN!
void colon(char* buf) {

// check how many addresses we got
#define GOT_ADDRESS (got & 1)
#define GOT_RANGE ((got & 3) == 3)

    char c, *buf1, *q, *r;
    char *fn, cmd[MAX_INPUT_LEN], *cmdend, *args, *exp = NULL;
    int i, l, li, b, e;
    uint32_t got;
    int useforce;

    // :3154    // if (-e line 3154) goto it  else stay put
    // :4,33w! foo    // write a portion of buffer to file "foo"
    // :w        // write all of buffer to current file
    // :q        // quit
    // :q!        // quit- dont care about modified file
    // :'a,'z!sort -u   // filter block through sort
    // :'f        // goto mark "f"
    // :'fl        // list literal the mark "f" line
    // :.r bar    // read file "bar" into buffer before dot
    // :/123/,/abc/d    // delete lines from "123" line to "abc" line
    // :/xyz/    // goto the "xyz" line
    // :s/find/replace/ // substitute pattern "find" with "replace"
    // :!<cmd>    // run <cmd> then return
    //

    while (*buf == ':')
        buf++; // move past leading colons
    while (isblank(*buf))
        buf++; // move past leading blanks
    if (!buf[0] || buf[0] == '"')
        goto ret; // ignore empty lines or those starting with '"'

    li = i = 0;
    b = e = -1;
    got = 0;
    li = count_lines(text, end - 1);
    fn = current_filename;

    // look for optional address(es)  :.  :1  :1,9   :'q,'a   :%
    buf = get_address(buf, &b, &e, &got);
    if (buf == NULL) {
        goto ret;
    }

    // get the COMMAND into cmd[]
    strcpy(cmd, buf);
    buf1 = cmd;
    while (!isspace(*buf1) && *buf1 != '\0') {
        buf1++;
    }
    cmdend = buf1;
    // get any ARGuments
    while (isblank(*buf1))
        buf1++;
    args = buf1;
    *cmdend = '\0';
    useforce = false;
    if (cmdend > cmd && cmdend[-1] == '!') {
        useforce = true;
        cmdend[-1] = '\0'; // get rid of !
    }
    // assume the command will want a range, certain commands
    // (read, substitute) need to adjust these assumptions
    if (!GOT_ADDRESS) {
        q = text; // no addr, use 1,$ for the range
        r = end - 1;
    } else {
        // at least one addr was given, get its details
        if (e < 0 || e > li) {
            status_line_bold("Invalid range");
            goto ret;
        }
        q = r = find_line(e);
        if (!GOT_RANGE) {
            // if there is only one addr, then it's the line
            // number of the single line the user wants.
            // Reset the end pointer to the end of that line.
            r = end_line(q);
            li = 1;
        } else {
            // we were given two addrs.  change the
            // start pointer to the addr given by user.
            if (b < 0 || b > li || b > e) {
                status_line_bold("Invalid range");
                goto ret;
            }
            q = find_line(b); // what line is #b
            r = end_line(r);
            li = e - b + 1;
        }
    }
    // ------------ now look for the command ------------
    i = strlen(cmd);
    if (i == 0) { // :123CR goto line #123
        if (e >= 0) {
            dot = find_line(e); // what line is #e
            dot_skip_over_ws();
        }
    } else if (cmd[0] == '=' && !cmd[1]) { // where is the address
        if (!GOT_ADDRESS) {                // no addr given- use defaults
            e = count_lines(text, dot);
        }
        status_line("%d", e);
    } else if (strncmp(cmd, "delete", i) == 0) { // delete lines
        if (!GOT_ADDRESS) {                      // no addr given- use defaults
            q = begin_line(dot);                 // assume .,. for the range
            r = end_line(dot);
        }
        dot = yank_delete(q, r, WHOLE, YANKDEL, ALLOW_UNDO); // save, then delete lines
        dot_skip_over_ws();
    } else if (strncmp(cmd, "edit", i) == 0) { // Edit a file
        int size;

        // don't edit, if the current file has been modified
        if (modified_count && !useforce) {
            status_line_bold("No write since last change (:%s! overrides)", cmd);
            goto ret;
        }
        if (args[0]) {
            // the user supplied a file name
            fn = exp = args;
        } else if (current_filename == NULL) {
            // no user file name, no current name- punt
            status_line_bold("No current filename");
            goto ret;
        }

        size = init_text_buffer(fn);

        if (Ureg >= 0 && Ureg < 28) {
            free(reg[Ureg]); //   free orig line reg- for 'U'
            reg[Ureg] = NULL;
        }
        /*if (YDreg < 28) - always true*/ {
            free(reg[YDreg]); //   free default yank/delete register
            reg[YDreg] = NULL;
        }
        // how many lines in text[]?
        li = count_lines(text, end - 1);
        status_line("'%s'%s"
                    " %uL, %uC",
                    fn, (size < 0 ? " [New file]" : ""), li, (int)(end - text));
    } else if (strncmp(cmd, "file", i) == 0) { // what File is this
        if (e >= 0) {
            status_line_bold("No address allowed on this command");
            goto ret;
        }
        if (args[0]) {
            // user wants a new filename
            exp = args;
            update_filename(exp);
        } else {
            // user wants file status info
            last_status_cksum = 0; // force status update
        }
    } else if (strncmp(cmd, "list", i) == 0) { // literal print line
        if (!GOT_ADDRESS) {                    // no addr given- use defaults
            q = begin_line(dot);               // assume .,. for the range
            r = end_line(dot);
        }
        go_bottom_and_clear_to_eol();
        printf("\r");
        for (; q <= r; q++) {
            int c_is_no_print;

            c = *q;
            c_is_no_print = (c & 0x80) && !is_asciionly(c);
            if (c_is_no_print) {
                c = '.';
                standout_start();
            }
            if (c == '\n') {
                puts_no_eol("$\r");
            } else if (c < ' ' || c == 127) {
                putchar('^');
                if (c == 127)
                    c = '?';
                else
                    c += '@';
            }
            putchar(c);
            if (c_is_no_print)
                standout_end();
        }
        Hit_Return();
    } else if (strncmp(cmd, "quit", i) == 0    // quit
               || strncmp(cmd, "next", i) == 0 // edit next file
               || strncmp(cmd, "prev", i) == 0 // edit previous file
    ) {
        int n;
        if (useforce) {
            editing = 0;
            goto ret;
        }
        // don't exit if the file been modified
        if (modified_count) {
            status_line_bold("No write since last change (:%s! overrides)", cmd);
            goto ret;
        }
        // are there other file to edit
        n = argc - vi_optind - 1;
        if (*cmd == 'q' && n > 0) {
            status_line_bold("%u more file(s) to edit", n);
            goto ret;
        }
        if (*cmd == 'n' && n <= 0) {
            status_line_bold("No more files to edit");
            goto ret;
        }
        if (*cmd == 'p') {
            // are there previous files to edit
            if (vi_optind < 2) {
                status_line_bold("No previous files to edit");
                goto ret;
            }
            vi_optind -= 2;
        }
        editing = 0;
    } else if (strncmp(cmd, "read", i) == 0) { // read file into text[]
        int size, num;

        if (args[0]) {
            // the user supplied a file name
            fn = exp = args;
            if (exp == NULL)
                goto ret;
            init_filename(fn);
        } else if (current_filename == NULL) {
            // no user file name, no current name- punt
            status_line_bold("No current filename");
            goto ret;
        }
        if (e == 0) { // user said ":0r foo"
            q = text;
        } else { // read after given line or current line if none given
            q = next_line(GOT_ADDRESS ? find_line(e) : dot);
            // read after last line
            if (q == end - 1)
                ++q;
        }
        num = count_lines(text, q);
        if (q == end)
            num++;
        { // dance around potentially-reallocated text[]
            intptr_t ofs = (intptr_t)(q - text);
            size = file_insert(fn, q, 0);
            q = text + ofs;
        }
        if (size < 0)
            goto ret; // nothing was inserted
        // how many lines in text[]?
        li = count_lines(q, q + size - 1);
        status_line("'%s' %uL, %uC", fn, li, size);
        dot = find_line(num);
    } else if (strncmp(cmd, "rewind", i) == 0) { // rewind cmd line args
        if (modified_count && !useforce) {
            status_line_bold("No write since last change (:%s! overrides)", cmd);
        } else {
            // reset the filenames to edit
            vi_optind = 0; // start from 0th file
            editing = 0;
        }
    } else if (strncmp(cmd, "set", i) == 0) { // set or clear features
        char *argp, *argn, oldch;
        // only blank is regarded as args delimiter. What about tab '\t'?
        if (!args[0] || strcmp(args, "all") == 0) {
            // print out values of all options
            status_line_bold("%sautoindent "
                             "%sexpandtab "
                             "%sflash "
                             "%signorecase "
                             "%sshowmatch "
                             "tabstop=%u",
                             autoindent ? "" : "no", expandtab ? "" : "no", err_method ? "" : "no",
                             ignorecase ? "" : "no", showmatch ? "" : "no", tabstop);
            goto ret;
        }
        argp = args;
        while (*argp) {
            i = 0;
            if (argp[0] == 'n' && argp[1] == 'o') // "noXXX"
                i = 2;
            argn = skip_non_whitespace(argp);
            oldch = *argn;
            *argn = '\0';
            setops(argp, i);
            *argn = oldch;
            argp = skip_whitespace(argn);
        }
    } else if (cmd[0] == 's') { // substitute a pattern with a replacement pattern
        char *F, *R, *flags;
        size_t len_F, len_R;
        int gflag = 0; // global replace flag
        int subs = 0;  // number of substitutions
        int last_line = 0, lines = 0;

        // F points to the "find" pattern
        // R points to the "replace" pattern
        // replace the cmd line delimiters "/" with NULs
        c = buf[1];                 // what is the delimiter
        F = buf + 2;                // start of "find"
        R = strchr_backslash(F, c); // middle delimiter
        if (!R)
            goto colon_s_fail;
        len_F = R - F;
        *R++ = '\0'; // terminate "find"
        flags = strchr_backslash(R, c);
        if (flags) {
            *flags++ = '\0'; // terminate "replace"
            gflag = *flags;
        }

        if (len_F) { // save "find" as last search pattern
            free(last_search_pattern);
            last_search_pattern = strdup(F - 1);
            last_search_pattern[0] = '/';
        } else if (last_search_pattern[1] == '\0') {
            status_line_bold("No previous search");
            goto ret;
        } else {
            F = last_search_pattern + 1;
            len_F = strlen(F);
        }

        if (!GOT_ADDRESS) {      // no addr given
            q = begin_line(dot); // start with cur line
            r = end_line(dot);
            b = e = count_lines(text, q); // cur line number
        } else if (!GOT_RANGE) {          // one addr given
            b = e;
        }

        len_R = strlen(R);

        for (i = b; i <= e; i++) { // so, :20,23 s \0 find \0 replace \0
            char* ls = q;          // orig line start
            char* found;
        vc4:
            found = char_search(q, F, (FORWARD << 1) | LIMITED); // search cur line only for "find"
            if (found) {
                intptr_t bias;
                // we found the "find" pattern - delete it
                // For undo support, the first item should not be chained
                // This needs to be handled differently depending on
                // whether or not regex support is enabled.
#define TEST_LEN_F 1 // len_F is never zero
#define TEST_UNDO1 subs
#define TEST_UNDO2 1
                if (TEST_LEN_F) // match can be empty, no delete needed
                    text_hole_delete(found, found + len_F - 1,
                                     TEST_UNDO1 ? ALLOW_UNDO_CHAIN : ALLOW_UNDO);
                if (len_R != 0) { // insert the "replace" pattern, if required
                    bias = string_insert(found, R, TEST_UNDO2 ? ALLOW_UNDO_CHAIN : ALLOW_UNDO);
                    found += bias;
                    ls += bias;
                    // q += bias; - recalculated anyway
                }
                if (TEST_LEN_F || len_R != 0) {
                    dot = ls;
                    subs++;
                    if (last_line != i) {
                        last_line = i;
                        ++lines;
                    }
                }
                // check for "global"  :s/foo/bar/g
                if (gflag == 'g') {
                    if ((found + len_R) < end_line(ls)) {
                        q = found + len_R;
                        goto vc4; // don't let q move past cur line
                    }
                }
            }
            q = next_line(ls);
        }
        if (subs == 0) {
            status_line_bold("No match");
        } else {
            dot_skip_over_ws();
            if (subs > 1)
                status_line("%d substitutions on %d lines", subs, lines);
        }
    } else if (strncmp(cmd, "version", i) == 0) { // show software version
        status_line("vi " VI_VER);
    } else if (strncmp(cmd, "write", i) == 0 // write text to file
               || strcmp(cmd, "wq") == 0 || strcmp(cmd, "wn") == 0 || (cmd[0] == 'x' && !cmd[1])) {
        int size;
        // int forced = false;

        // is there a file name to write to?
        if (args[0]) {
            struct stat statbuf;

            exp = args;
            if (!useforce && (fn == NULL || strcmp(fn, exp) != 0) && stat(exp, &statbuf) >= 0) {
                status_line_bold("File exists (:w! overrides)");
                goto ret;
            }
            fn = exp;
            init_filename(fn);
        }
        // if (useforce) {
        // if "fn" is not write-able, chmod u+w
        // sprintf(syscmd, "chmod u+w %s", fn);
        // system(syscmd);
        // forced = true;
        //}
        if (modified_count != 0 || cmd[0] != 'x') {
            size = r - q + 1;
            l = file_write(fn, q, r);
        } else {
            size = 0;
            l = 0;
        }
        // if (useforce && forced) {
        // chmod u-w
        // sprintf(syscmd, "chmod u-w %s", fn);
        // system(syscmd);
        // forced = false;
        //}
        if (l < 0) {
            if (l == -1)
                status_line_bold_errno(fn);
        } else {
            // how many lines written
            li = count_lines(q, q + l - 1);
            status_line("'%s' %uL, %uC", fn, li, l);
            if (l == size) {
                if (q == text && q + l == end) {
                    modified_count = 0;
                    last_modified_count = -1;
                }
                if (cmd[1] == 'n') {
                    editing = 0;
                } else if (cmd[0] == 'x' || cmd[1] == 'q') {
                    // are there other files to edit?
                    int n = argc - vi_optind - 1;
                    if (n > 0) {
                        if (useforce) {
                            // force end of argv list
                            vi_optind = argc;
                        } else {
                            status_line_bold("%u more file(s) to edit", n);
                            goto ret;
                        }
                    }
                    editing = 0;
                }
            }
        }
    } else if (strncmp(cmd, "yank", i) == 0) { // yank lines
        if (!GOT_ADDRESS) {                    // no addr given- use defaults
            q = begin_line(dot);               // assume .,. for the range
            r = end_line(dot);
        }
        text_yank(q, r, YDreg, WHOLE);
        li = count_lines(q, r);
        status_line("Yank %d lines (%d chars) into [%c]", li, strlen(reg[YDreg]), what_reg());
    } else {
        // cmd unknown
        not_implemented(cmd);
    }
ret:
    dot = bound_dot(dot); // make sure "dot" is valid
    return;
colon_s_fail:
    status_line(":s expression missing delimiters");
}

//----- Char Routines --------------------------------------------
// Chars that are part of a word-
//    0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
// Chars that are Not part of a word (stoppers)
//    !"#$%&'()*+,-./:;<=>?@[\]^`{|}~
// Chars that are WhiteSpace
//    TAB NEWLINE VT FF RETURN SPACE
// DO NOT COUNT NEWLINE AS WHITESPACE

int st_test(char* p, int type, int dir, char* tested) {
    char c, c0, ci;
    int test, inc;

    inc = dir;
    c = c0 = p[0];
    ci = p[inc];
    test = 0;

    if (type == S_BEFORE_WS) {
        c = ci;
        test = (!isspace(c) || c == '\n');
    }
    if (type == S_TO_WS) {
        c = c0;
        test = (!isspace(c) || c == '\n');
    }
    if (type == S_OVER_WS) {
        c = c0;
        test = isspace(c);
    }
    if (type == S_END_PUNCT) {
        c = ci;
        test = ispunct(c);
    }
    if (type == S_END_ALNUM) {
        c = ci;
        test = (isalnum(c) || c == '_');
    }
    *tested = c;
    return test;
}

char* skip_thing(char* p, int linecnt, int dir, int type) {
    char c;

    while (st_test(p, type, dir, &c)) {
        // make sure we limit search to correct number of lines
        if (c == '\n' && --linecnt < 1)
            break;
        if (dir >= 0 && p >= end - 1)
            break;
        if (dir < 0 && p <= text)
            break;
        p += dir; // move to next char
    }
    return p;
}

void do_cmd(int c);

int at_eof(const char* s) {
    // does 's' point to end of file, even with no terminating newline?
    return ((s == end - 2 && s[1] == '\n') || s == end - 1);
}

int find_range(char** start, char** stop, int cmd) {
    char *p, *q, *t;
    int buftype = -1;
    int c;

    p = q = dot;

    if (cmd == 'Y') {
        c = 'y';
    } else {
        c = get_motion_char();
    }

    if ((cmd == 'Y' || cmd == c) && strchr("cdy><", c)) {
        // these cmds operate on whole lines
        buftype = WHOLE;
        if (--cmdcnt > 0) {
            do_cmd('j');
            if (cmd_error)
                buftype = -1;
        }
    } else if (strchr("^%$0bBeEfFtThnN/?|{}\b\177", c)) {
        // Most operate on char positions within a line.  Of those that
        // don't '%' needs no special treatment, search commands are
        // marked as MULTI and  "{}" are handled below.
        buftype = strchr("nN/?", c) ? MULTI : PARTIAL;
        do_cmd(c);    // execute movement cmd
        if (p == dot) // no movement is an error
            buftype = -1;
    } else if (strchr("wW", c)) {
        buftype = MULTI;
        do_cmd(c); // execute movement cmd
        // step back one char, but not if we're at end of file,
        // or if we are at EOF and search was for 'w' and we're at
        // the start of a 'W' word.
        if (dot > p && (!at_eof(dot) || (c == 'w' && ispunct(*dot))))
            dot--;
        t = dot;
        // don't include trailing WS as part of word
        while (dot > p && isspace(*dot)) {
            if (*dot-- == '\n')
                t = dot;
        }
        // for non-change operations WS after NL is not part of word
        if (cmd != 'c' && dot != t && *dot != '\n')
            dot = t;
    } else if (strchr("GHL+-gjk'\r\n", c)) {
        // these operate on whole lines
        buftype = WHOLE;
        do_cmd(c); // execute movement cmd
        if (cmd_error)
            buftype = -1;
    } else if (c == ' ' || c == 'l') {
        // forward motion by character
        int tmpcnt = (cmdcnt ?: 1);
        buftype = PARTIAL;
        do_cmd(c); // execute movement cmd
        // exclude last char unless range isn't what we expected
        // this indicates we've hit EOL
        if (tmpcnt == dot - p)
            dot--;
    }

    if (buftype == -1) {
        if (c != 27)
            indicate_error();
        return buftype;
    }

    q = dot;
    if (q < p) {
        t = q;
        q = p;
        p = t;
    }

    // movements which don't include end of range
    if (q > p) {
        if (strchr("^0bBFThnN/?|\b\177", c)) {
            q--;
        } else if (strchr("{}", c)) {
            buftype = (p == begin_line(p) && (*q == '\n' || at_eof(q))) ? WHOLE : MULTI;
            if (!at_eof(q)) {
                q--;
                if (q > p && p != begin_line(p))
                    q--;
            }
        }
    }

    *start = p;
    *stop = q;
    return buftype;
}

