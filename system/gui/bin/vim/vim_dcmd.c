/* vim_dcmd.c — normal mode command dispatch, do_cmd() (split from vi.c) */
#include "vim.h"

//---------------------------------------------------------------------
//----- the Ascii Chart -----------------------------------------------
//  00 nul   01 soh   02 stx   03 etx   04 eot   05 enq   06 ack   07 bel
//  08 bs    09 ht    0a nl    0b vt    0c np    0d cr    0e so    0f si
//  10 dle   11 dc1   12 dc2   13 dc3   14 dc4   15 nak   16 syn   17 etb
//  18 can   19 em    1a sub   1b esc   1c fs    1d gs    1e rs    1f us
//  20 sp    21 !     22 "     23 #     24 $     25 %     26 &     27 '
//  28 (     29 )     2a *     2b +     2c ,     2d -     2e .     2f /
//  30 0     31 1     32 2     33 3     34 4     35 5     36 6     37 7
//  38 8     39 9     3a :     3b ;     3c <     3d =     3e >     3f ?
//  40 @     41 A     42 B     43 C     44 D     45 E     46 F     47 G
//  48 H     49 I     4a J     4b K     4c L     4d M     4e N     4f O
//  50 P     51 Q     52 R     53 S     54 T     55 U     56 V     57 W
//  58 X     59 Y     5a Z     5b [     5c \     5d ]     5e ^     5f _
//  60 `     61 a     62 b     63 c     64 d     65 e     66 f     67 g
//  68 h     69 i     6a j     6b k     6c l     6d m     6e n     6f o
//  70 p     71 q     72 r     73 s     74 t     75 u     76 v     77 w
//  78 x     79 y     7a z     7b {     7c |     7d }     7e ~     7f del
//---------------------------------------------------------------------

//----- Visual (charwise) selection -------------------------------------
// 'v' anchors a selection at "dot"; motions extend it, an operator key
// applies to the whole selection, anything else cancels it.

// keys that keep the selection alive: motions, counts and scroll/redraw
static bool is_visual_motion(int c) {
    switch (c) {
    case KEYCODE_UP:
    case KEYCODE_DOWN:
    case KEYCODE_LEFT:
    case KEYCODE_RIGHT:
    case KEYCODE_HOME:
    case KEYCODE_END:
    case KEYCODE_PAGEUP:
    case KEYCODE_PAGEDOWN:
    case 2:    // ctrl-B  scroll up full screen
    case 4:    // ctrl-D  scroll down half screen
    case 5:    // ctrl-E  scroll down one line
    case 6:    // ctrl-F  scroll down full screen
    case 8:    // ctrl-H  move left
    case 0x7f: // DEL     move left
    case 12:   // ctrl-L  redraw
    case 18:   // ctrl-R  redraw
    case 21:   // ctrl-U  scroll up half screen
    case 25:   // ctrl-Y  scroll up one line
        return true;
    default:
        // digits accumulate cmdcnt, '"' prefixes a register for y/d/c
        return c > 0 && strchr("hjkl \r\n+-0$^%|wWbBeE{}gGHLMfFtT;,/?nNz\"123456789", c) != NULL;
    }
}

// apply operator c to the selection and leave visual mode
static void visual_operate(int c) {
    char *lo = vi_visual_anchor, *hi = dot, *t, *p;
    int linewise = (c == 'X' || c == 'Y' || c == 'D' || c == 'C' || c == '>' || c == '<');

    if (lo > hi) {
        t = lo;
        lo = hi;
        hi = t;
    }
    if (lo < text)
        lo = text;
    if (hi > end - 1)
        hi = end - 1;
    vi_visual = 0;
    last_status_cksum = 0; // force status update

    if (c == '>' || c == '<') { // shift the selected lines left/right
        int li = count_lines(text, lo);   // remember what line the range starts on
        int nlines = count_lines(lo, hi); // # of lines we are shifting
        int allow_undo = ALLOW_UNDO;
        int j;
        for (p = begin_line(lo); nlines > 0; nlines--, p = next_line(p)) {
            if (c == '<') {
                // shift left- remove tab or tabstop spaces
                if (*p == '\t') {
                    p = text_hole_delete(p, p, allow_undo);
                } else if (*p == ' ') {
                    for (j = 0; *p == ' ' && j < tabstop; j++) {
                        p = text_hole_delete(p, p, allow_undo);
                        allow_undo = ALLOW_UNDO_CHAIN;
                    }
                }
            } else if (p != end_line(p)) {
                // shift right -- add tab or tabstop spaces on non-empty lines
                p = char_insert(p, '\t', allow_undo);
            }
            allow_undo = ALLOW_UNDO_CHAIN;
        }
        dot = find_line(li); // go back to the line the selection started on
        dot_skip_over_ws();
    } else {
        int yf = (c == 'y' || c == 'Y') ? YANKONLY : YANKDEL;
        int buftype =
            linewise ? WHOLE : (begin_line(lo) == begin_line(hi) ? PARTIAL : MULTI);
        char* savereg = reg[YDreg]; // yank_delete() may refuse a lone newline
        dot = yank_delete(lo, hi, buftype, yf, ALLOW_UNDO);
        if (linewise) {
            if (c == 'C') { // like 'cc': leave one empty line behind
                dot = char_insert(dot, '\n', ALLOW_UNDO_CHAIN);
                if (dot != (end - 1))
                    dot_prev();
            } else {
                dot_begin();
                dot_skip_over_ws();
            }
        }
        if (reg[YDreg] != savereg)
            yank_status(yf == YANKONLY ? "Yank" : "Delete", reg[YDreg], 1);
        if (c == 'c' || c == 'C') {
            cmd_mode = 1;        // start inserting, as 'c' does
            undo_queue_commit(); // commit queue when cmd_mode changes
        }
    }
    end_cmd_q(); // stop adding to q
}

//----- Execute a Vi Command -----------------------------------
void do_cmd(int c) {
    char *p, *q, *save_dot;
    char buf[12];
    int dir;
    int cnt, i, j;
    int c1;
    char* orig_dot = dot;
    int allow_undo = ALLOW_UNDO;
    int undo_del = UNDO_DEL;

    //    c1 = c; // quiet the compiler
    //    cnt = yf = 0; // quiet the compiler
    //    p = q = save_dot = buf; // quiet the compiler
    memset(buf, 0, sizeof(buf));
    keep_index = false;
    cmd_error = false;

    show_status_line();

    // if this is a cursor key, skip these checks
    switch (c) {
    case KEYCODE_UP:
    case KEYCODE_DOWN:
    case KEYCODE_LEFT:
    case KEYCODE_RIGHT:
    case KEYCODE_HOME:
    case KEYCODE_END:
    case KEYCODE_PAGEUP:
    case KEYCODE_PAGEDOWN:
    case KEYCODE_DELETE:
        goto key_cmd_mode;
    }

    if (cmd_mode == 2) {
        //  flip-flop Insert/Replace mode
        if (c == KEYCODE_INSERT)
            goto dc_i;
        // we are 'R'eplacing the current *dot with new char
        if (*dot == '\n') {
            // don't Replace past E-o-l
            cmd_mode = 1; // convert to insert
            undo_queue_commit();
        } else {
            if (1 <= c || is_asciionly(c)) {
                if (c != 27)
                    dot = yank_delete(dot, dot, PARTIAL, YANKDEL, ALLOW_UNDO); // delete char
                dot = char_insert(dot, c, ALLOW_UNDO_CHAIN);                   // insert new char
            }
            goto dc1;
        }
    }
    if (cmd_mode == 1) {
        // hitting "Insert" twice means "R" replace mode
        if (c == KEYCODE_INSERT)
            goto dc5;
        // insert the char c at "dot"
        if (1 <= c || is_asciionly(c)) {
            dot = char_insert(dot, c, ALLOW_UNDO_QUEUED);
        }
        goto dc1;
    }

key_cmd_mode:
    // a pending visual selection: motions extend it, operator keys apply to
    // it, 'v'/ESC abandon it, any other key cancels it and runs normally
    if (vi_visual && cmd_mode == 0) {
        if (c == 27 || c == 'v') { // abandon the selection
            vi_visual = 0;
            end_cmd_q();           // stop adding to q
            last_status_cksum = 0; // force status update
            goto dc1;
        }
        if (c == KEYCODE_DELETE || (c > 0 && strchr("yYdDxXcC><", c) != NULL)) {
            visual_operate(c);
            goto dc1;
        }
        if (!is_visual_motion(c)) {
            vi_visual = 0;
            last_status_cksum = 0; // force status update
        }
    }
    switch (c) {
    default: // unrecognized command
        buf[0] = c;
        buf[1] = '\0';
        not_implemented(buf);
        end_cmd_q(); // stop adding to q
    case 0x00:       // nul- ignore
        break;
    case 2:              // ctrl-B  scroll up   full screen
    case KEYCODE_PAGEUP: // Cursor Key Page Up
        dot_scroll(rows - 2, -1);
        break;
    case 4: // ctrl-D  scroll down half screen
        dot_scroll((rows - 2) / 2, 1);
        break;
    case 5: // ctrl-E  scroll down one line
        dot_scroll(1, 1);
        break;
    case 6:                // ctrl-F  scroll down full screen
    case KEYCODE_PAGEDOWN: // Cursor Key Page Down
        dot_scroll(rows - 2, 1);
        break;
    case 7:                    // ctrl-G  show current status
        last_status_cksum = 0; // force status update
        break;
    case 'h':          // h- move left
    case KEYCODE_LEFT: // cursor key Left
    case 8:            // ctrl-H- move left    (This may be ERASE char)
    case 0x7f:         // DEL- move left   (This may be ERASE char)
        do {
            dot_left();
        } while (--cmdcnt > 0);
        break;
    case 10:           // Newline ^J
    case 'j':          // j- goto next line, same col
    case KEYCODE_DOWN: // cursor key Down
    case 13:           // Carriage Return ^M
    case '+':          // +- goto next line
        q = dot;
        do {
            p = next_line(q);
            if (p == end_line(q)) {
                indicate_error();
                goto dc1;
            }
            q = p;
        } while (--cmdcnt > 0);
        dot = q;
        if (c == 13 || c == '+') {
            dot_skip_over_ws();
        } else {
            // try to stay in saved column
            dot = cindex == C_END ? end_line(dot) : move_to_col(dot, cindex);
            keep_index = true;
        }
        break;
    case 12:          // ctrl-L  force redraw whole screen
    case 18:          // ctrl-R  force redraw
        redraw(true); // this will redraw the entire display
        break;
    case 21: // ctrl-U  scroll up half screen
        dot_scroll((rows - 2) / 2, -1);
        break;
    case 25: // ctrl-Y  scroll up one line
        dot_scroll(1, -1);
        break;
    case 27: // esc
        if (cmd_mode == 0)
            indicate_error();
        cmd_mode = 0; // stop inserting
        undo_queue_commit();
        end_cmd_q();
        last_status_cksum = 0; // force status update
        break;
    case ' ':           // move right
    case 'l':           // move right
    case KEYCODE_RIGHT: // Cursor Key Right
        do {
            dot_right();
        } while (--cmdcnt > 0);
        break;
    case '"':                               // "- name a register to use for Delete/Yank
        c1 = (get_one_char() | 0x20) - 'a'; // | 0x20 is tolower()
        if ((uint32_t)c1 <= 25) {           // a-z?
            YDreg = c1;
        } else {
            indicate_error();
        }
        break;
    case '\'': // '- goto a specific mark
        c1 = (get_one_char() | 0x20);
        if ((uint32_t)(c1 - 'a') <= 25) { // a-z?
            c1 = (c1 - 'a');
            // get the b-o-l
            q = mark[c1];
            if (text <= q && q < end) {
                dot = q;
                dot_begin(); // go to B-o-l
                dot_skip_over_ws();
            } else {
                indicate_error();
            }
        } else if (c1 == '\'') {     // goto previous context
            dot = swap_context(dot); // swap current and previous context
            dot_begin();             // go to B-o-l
            dot_skip_over_ws();
            orig_dot = dot; // this doesn't update stored contexts
        } else {
            indicate_error();
        }
        break;
    case 'm': // m- Mark a line
        // this is really stupid.  If there are any inserts or deletes
        // between text[0] and dot then this mark will not point to the
        // correct location! It could be off by many lines!
        // Well..., at least its quick and dirty.
        c1 = (get_one_char() | 0x20) - 'a';
        if ((uint32_t)c1 <= 25) { // a-z?
            // remember the line
            mark[c1] = dot;
        } else {
            indicate_error();
        }
        break;
    case 'P': // P- Put register before
    case 'p': // p- put register after
        p = reg[YDreg];
        if (p == NULL) {
            status_line_bold("Nothing in register %c", what_reg());
            break;
        }
        cnt = 0;
        i = cmdcnt ?: 1;
        // are we putting whole lines or strings
        if (regtype[YDreg] == WHOLE) {
            if (c == 'P') {
                dot_begin(); // putting lines- Put above
            } else /* if ( c == 'p') */ {
                // are we putting after very last line?
                if (end_line(dot) == (end - 1)) {
                    dot = end; // force dot to end of text[]
                } else {
                    dot_next(); // next line, then put before
                }
            }
        } else {
            if (c == 'p')
                dot_right(); // move to right, can move to NL
            // how far to move cursor if register doesn't have a NL
            if (strchr(p, '\n') == NULL)
                cnt = i * strlen(p) - 1;
        }
        do {
            // dot is adjusted if text[] is reallocated so we don't have to
            string_insert(dot, p, allow_undo); // insert the string
            allow_undo = ALLOW_UNDO_CHAIN;
        } while (--cmdcnt > 0);
        dot += cnt;
        dot_skip_over_ws();
        yank_status("Put", p, i);
        end_cmd_q(); // stop adding to q
        break;
    case 'U': // U- Undo; replace current line with original version
        if (reg[Ureg] != NULL) {
            p = begin_line(dot);
            q = end_line(dot);
            p = text_hole_delete(p, q, ALLOW_UNDO);             // delete cur line
            p += string_insert(p, reg[Ureg], ALLOW_UNDO_CHAIN); // insert orig line
            dot = p;
            dot_skip_over_ws();
            yank_status("Undo", reg[Ureg], 1);
        }
        break;
    case 'u': // u- undo last operation
        undo_pop();
        break;
    case 'v': // v- start a charwise visual selection ('v' again cancels it)
        vi_visual = 1;
        vi_visual_anchor = dot;
        last_status_cksum = 0; // force status update
        break;
    case '$':         // $- goto end of line
    case KEYCODE_END: // Cursor Key End
        for (;;) {
            dot = end_line(dot);
            if (--cmdcnt <= 0)
                break;
            dot_next();
        }
        cindex = C_END;
        keep_index = true;
        break;
    case '%': // %- find matching char of pair () [] {}
        for (q = dot; q < end && *q != '\n'; q++) {
            if (strchr("()[]{}", *q) != NULL) {
                // we found half of a pair
                p = find_pair(q, *q);
                if (p == NULL) {
                    indicate_error();
                } else {
                    dot = p;
                }
                break;
            }
        }
        if (*q == '\n')
            indicate_error();
        break;
    case 'f':                              // f- forward to a user specified char
    case 'F':                              // F- backward to a user specified char
    case 't':                              // t- move to char prior to next x
    case 'T':                              // T- move to char after previous x
        last_search_char = get_one_char(); // get the search char
        last_search_cmd = c;
        // fall through
    case ';': // ;- look at rest of line for last search char
    case ',': // ,- repeat latest search in opposite direction
        dot_to_char(c != ',' ? last_search_cmd : last_search_cmd ^ 0x20);
        break;
    case '.': // .- repeat the last modifying command
        // Stuff the last_modifying_cmd back into stdin
        // and let it be re-executed.
        if (lmc_len != 0) {
            if (cmdcnt) // update saved count if current count is non-zero
                dotcnt = cmdcnt;
            last_modifying_cmd[lmc_len] = '\0';
            ioq = ioq_start = xvsnprintf("%u%s", dotcnt, last_modifying_cmd);
        }
        break;
    case 'N': // N- backward search for last pattern
        dir = last_search_pattern[0] == '/' ? BACK : FORWARD;
        goto dc4; // now search for pattern
        break;
    case '?': // ?- backward search for a pattern
    case '/': // /- forward search for a pattern
        buf[0] = c;
        buf[1] = '\0';
        q = get_input_line(buf); // get input line- use "status line"
        if (!q[0])               // user changed mind and erased the "/"-  do nothing
            break;
        if (!q[1]) { // if no pat re-use old pat
            if (last_search_pattern[0])
                last_search_pattern[0] = c;
        } else { // strlen(q) > 1: new pat- save it and find
            free(last_search_pattern);
            last_search_pattern = strdup(q);
        }
        // fall through
    case 'n': // n- repeat search for last pattern
        // search rest of text[] starting at next char
        // if search fails "dot" is unchanged
        dir = last_search_pattern[0] == '/' ? FORWARD : BACK;
    dc4:
        if (last_search_pattern[1] == '\0') {
            status_line_bold("No previous search");
            break;
        }
        do {
            q = char_search(dot + dir, last_search_pattern + 1, (dir << 1) | FULL);
            if (q != NULL) {
                dot = q; // good search, update "dot"
            } else {
                // no pattern found between "dot" and top/bottom of file
                // continue from other end of file
                const char* msg;
                q = char_search(dir == FORWARD ? text : end - 1, last_search_pattern + 1,
                                (dir << 1) | FULL);
                if (q != NULL) { // found something
                    dot = q;     // found new pattern- goto it
                    msg = "search hit %s, continuing at %s";
                } else {        // pattern is nowhere in file
                    cmdcnt = 0; // force exit from loop
                    msg = "Pattern not found";
                }
                if (dir == FORWARD)
                    status_line_bold(msg, "BOTTOM", "TOP");
                else
                    status_line_bold(msg, "TOP", "BOTTOM");
            }
        } while (--cmdcnt > 0);
        break;
    case '{': // {- move backward paragraph
    case '}': // }- move forward paragraph
        dir = c == '}' ? FORWARD : BACK;
        do {
            int skip = true; // initially skip consecutive empty lines
            while (dir == FORWARD ? dot < end - 1 : dot > text) {
                if (*dot == '\n' && dot[dir] == '\n') {
                    if (!skip) {
                        if (dir == FORWARD)
                            ++dot; // move to next blank line
                        goto dc2;
                    }
                } else {
                    skip = false;
                }
                dot += dir;
            }
            goto dc6; // end of file
        dc2:
            continue;
        } while (--cmdcnt > 0);
        break;
    case '0': // 0- goto beginning of line
    case '1': // 1-
    case '2': // 2-
    case '3': // 3-
    case '4': // 4-
    case '5': // 5-
    case '6': // 6-
    case '7': // 7-
    case '8': // 8-
    case '9': // 9-
        if (c == '0' && cmdcnt < 1) {
            dot_begin(); // this was a standalone zero
        } else {
            cmdcnt = cmdcnt * 10 + (c - '0'); // this 0 is part of a number
        }
        break;
    case ':':                    // :- the colon mode commands
        p = get_input_line(":"); // get input line- use "status line"
        colon(p);                // execute the command
        show_status_line();
        break;
    case '<':                         // <- Left  shift something
    case '>':                         // >- Right shift something
        cnt = count_lines(text, dot); // remember what line we are on
        if (find_range(&p, &q, c) == -1)
            goto dc6;
        i = count_lines(p, q); // # of lines we are shifting
        for (p = begin_line(p); i > 0; i--, p = next_line(p)) {
            if (c == '<') {
                // shift left- remove tab or tabstop spaces
                if (*p == '\t') {
                    // shrink buffer 1 char
                    text_hole_delete(p, p, allow_undo);
                } else if (*p == ' ') {
                    // we should be calculating columns, not just SPACE
                    for (j = 0; *p == ' ' && j < tabstop; j++) {
                        text_hole_delete(p, p, allow_undo);
                        allow_undo = ALLOW_UNDO_CHAIN;
                    }
                }
            } else if (/* c == '>' && */ p != end_line(p)) {
                // shift right -- add tab or tabstop spaces on non-empty lines
                char_insert(p, '\t', allow_undo);
            }
            allow_undo = ALLOW_UNDO_CHAIN;
        }
        dot = find_line(cnt); // what line were we on
        dot_skip_over_ws();
        end_cmd_q(); // stop adding to q
        break;
    case 'A':      // A- append at e-o-l
        dot_end(); // go to e-o-l
                   //**** fall through to ... 'a'
    case 'a':      // a- append after current char
        if (*dot != '\n')
            dot++;
        goto dc_i;
        break;
    case 'B': // B- back a blank-delimited Word
    case 'E': // E- end of a blank-delimited word
    case 'W': // W- forward a blank-delimited word
        dir = FORWARD;
        if (c == 'B')
            dir = BACK;
        do {
            if (c == 'W' || isspace(dot[dir])) {
                dot = skip_thing(dot, 1, dir, S_TO_WS);
                dot = skip_thing(dot, 2, dir, S_OVER_WS);
            }
            if (c != 'W')
                dot = skip_thing(dot, 1, dir, S_BEFORE_WS);
        } while (--cmdcnt > 0);
        break;
    case 'C': // C- Change to e-o-l
    case 'D': // D- delete to e-o-l
        save_dot = dot;
        dot = dollar_line(dot); // move to before NL
        // copy text into a register and delete
        dot = yank_delete(save_dot, dot, PARTIAL, YANKDEL, ALLOW_UNDO); // delete to e-o-l
        if (c == 'C')
            goto dc_i; // start inserting
        if (c == 'D')
            end_cmd_q(); // stop adding to q
        break;
    case 'g': // 'gg' goto a line number (vim) (default: very first line)
        c1 = get_one_char();
        if (c1 != 'g') {
            buf[0] = 'g';
            // c1 < 0 if the key was special. Try "g<up-arrow>"
            buf[1] = (c1 >= 0 ? c1 : '*');
            buf[2] = '\0';
            not_implemented(buf);
            cmd_error = true;
            break;
        }
        if (cmdcnt == 0)
            cmdcnt = 1;
        // fall through
    case 'G':          // G- goto to a line number (default= E-O-F)
        dot = end - 1; // assume E-O-F
        if (cmdcnt > 0) {
            dot = find_line(cmdcnt); // what line is #cmdcnt
        }
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'H': // H- goto top line on screen
        dot = screenbegin;
        if (cmdcnt > (rows - 1)) {
            cmdcnt = (rows - 1);
        }
        while (--cmdcnt > 0) {
            dot_next();
        }
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'I':        // I- insert before first non-blank
        dot_begin(); // 0
        dot_skip_over_ws();
        //**** fall through to ... 'i'
    case 'i':            // i- insert before current char
    case KEYCODE_INSERT: // Cursor Key Insert
    dc_i:
        cmd_mode = 1;        // start inserting
        undo_queue_commit(); // commit queue when cmd_mode changes
        break;
    case 'J': // J- join current and next lines together
        do {
            dot_end();           // move to NL
            if (dot < end - 1) { // make sure not last char in text[]
                undo_push(dot, 1, UNDO_DEL);
                *dot++ = ' '; // replace NL with space
                undo_push((dot - 1), 1, UNDO_INS_CHAIN);
                while (isblank(*dot)) { // delete leading WS
                    text_hole_delete(dot, dot, ALLOW_UNDO_CHAIN);
                }
            }
        } while (--cmdcnt > 0);
        end_cmd_q(); // stop adding to q
        break;
    case 'L': // L- goto bottom line on screen
        dot = end_screen();
        if (cmdcnt > (rows - 1)) {
            cmdcnt = (rows - 1);
        }
        while (--cmdcnt > 0) {
            dot_prev();
        }
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'M': // M- goto middle line on screen
        dot = screenbegin;
        for (cnt = 0; cnt < (rows - 1) / 2; cnt++)
            dot = next_line(dot);
        dot_skip_over_ws();
        break;
    case 'O': // O- open an empty line above
        dot_begin();
        indentcol = -1;
        goto dc3;
    case 'o': // o- open an empty line below
        dot_end();
    dc3:
        dot = char_insert(dot, '\n', ALLOW_UNDO);
        if (c == 'O' && !autoindent) {
            // done in char_insert() for 'O'+autoindent
            dot_prev();
        }
        goto dc_i;
        break;
    case 'R': // R- continuous Replace char
    dc5:
        cmd_mode = 2;
        undo_queue_commit();
        break;
    case KEYCODE_DELETE:
        if (dot < end - 1)
            dot = yank_delete(dot, dot, PARTIAL, YANKDEL, ALLOW_UNDO);
        break;
    case 'X': // X- delete char before dot
    case 'x': // x- delete the current char
    case 's': // s- substitute the current char
        dir = 0;
        if (c == 'X')
            dir = -1;
        do {
            if (dot[dir] != '\n') {
                if (c == 'X')
                    dot--;                                                 // delete prev char
                dot = yank_delete(dot, dot, PARTIAL, YANKDEL, allow_undo); // delete char
                allow_undo = ALLOW_UNDO_CHAIN;
            }
        } while (--cmdcnt > 0);
        end_cmd_q(); // stop adding to q
        if (c == 's')
            goto dc_i; // start inserting
        break;
    case 'Z': // Z- if modified, {write}; exit
        // ZZ means to save file (if necessary), then exit
        c1 = get_one_char();
        if (c1 != 'Z') {
            indicate_error();
            break;
        }
        if (modified_count) {
            cnt = file_write(current_filename, text, end - 1);
            if (cnt < 0) {
                if (cnt == -1)
                    status_line_bold("Write error: %s", strerror(errno));
            } else if (cnt == (end - 1 - text + 1)) {
                editing = 0;
            }
        } else {
            editing = 0;
        }
        // are there other files to edit?
        j = argc - vi_optind - 1;
        if (editing == 0 && j > 0) {
            editing = 1;
            modified_count = 0;
            last_modified_count = -1;
            status_line_bold("%u more file(s) to edit", j);
        }
        break;
    case '^': // ^- move to first non-blank on line
        dot_begin();
        dot_skip_over_ws();
        break;
    case 'b': // b- back a word
    case 'e': // e- end of word
        dir = FORWARD;
        if (c == 'b')
            dir = BACK;
        do {
            if ((dot + dir) < text || (dot + dir) > end - 1)
                break;
            dot += dir;
            if (isspace(*dot)) {
                dot = skip_thing(dot, (c == 'e') ? 2 : 1, dir, S_OVER_WS);
            }
            if (isalnum(*dot) || *dot == '_') {
                dot = skip_thing(dot, 1, dir, S_END_ALNUM);
            } else if (ispunct(*dot)) {
                dot = skip_thing(dot, 1, dir, S_END_PUNCT);
            }
        } while (--cmdcnt > 0);
        break;
    case 'c': // c- change something
    case 'd': // d- delete something
    case 'y': // y- yank   something
    case 'Y': // Y- Yank a line
    {
        int yf = YANKDEL; // assume either "c" or "d"
        int buftype;
        char* savereg = reg[YDreg];
        if (c == 'y' || c == 'Y')
            yf = YANKONLY;
        // determine range, and whether it spans lines
        buftype = find_range(&p, &q, c);
        if (buftype == -1) // invalid range
            goto dc6;
        if (buftype == WHOLE) {
            save_dot = p; // final cursor position is start of range
            p = begin_line(p);
            q = end_line(q);
        }
        dot = yank_delete(p, q, buftype, yf, ALLOW_UNDO); // delete word
        if (buftype == WHOLE) {
            if (c == 'c') {
                dot = char_insert(dot, '\n', ALLOW_UNDO_CHAIN);
                // on the last line of file don't move to prev line
                if (dot != (end - 1)) {
                    dot_prev();
                }
            } else if (c == 'd') {
                dot_begin();
                dot_skip_over_ws();
            } else {
                dot = save_dot;
            }
        }
        // if CHANGING, not deleting, start inserting after the delete
        if (c == 'c') {
            goto dc_i; // start inserting
        }
        // only update status if a yank has actually happened
        if (reg[YDreg] != savereg)
            yank_status(c == 'd' ? "Delete" : "Yank", reg[YDreg], 1);
    dc6:
        end_cmd_q(); // stop adding to q
        break;
    }
    case 'k':        // k- goto prev line, same col
    case KEYCODE_UP: // cursor key Up
    case '-':        // -- goto prev line
        q = dot;
        do {
            p = prev_line(q);
            if (p == begin_line(q)) {
                indicate_error();
                goto dc1;
            }
            q = p;
        } while (--cmdcnt > 0);
        dot = q;
        if (c == '-') {
            dot_skip_over_ws();
        } else {
            // try to stay in saved column
            dot = cindex == C_END ? end_line(dot) : move_to_col(dot, cindex);
            keep_index = true;
        }
        break;
    case 'r':                // r- replace the current char with user input
        c1 = get_one_char(); // get the replacement char
        if (c1 != 27) {
            if (end_line(dot) - dot < (cmdcnt ?: 1)) {
                indicate_error();
                goto dc6;
            }
            do {
                dot = text_hole_delete(dot, dot, allow_undo);
                allow_undo = ALLOW_UNDO_CHAIN;
                dot = char_insert(dot, c1, allow_undo);
            } while (--cmdcnt > 0);
            dot_left();
        }
        end_cmd_q(); // stop adding to q
        break;
    case 'w': // w- forward a word
        do {
            if (isalnum(*dot) || *dot == '_') { // we are on ALNUM
                dot = skip_thing(dot, 1, FORWARD, S_END_ALNUM);
            } else if (ispunct(*dot)) { // we are on PUNCT
                dot = skip_thing(dot, 1, FORWARD, S_END_PUNCT);
            }
            if (dot < end - 1)
                dot++; // move over word
            if (isspace(*dot)) {
                dot = skip_thing(dot, 2, FORWARD, S_OVER_WS);
            }
        } while (--cmdcnt > 0);
        break;
    case 'z':                // z-
        c1 = get_one_char(); // get the replacement char
        cnt = 0;
        if (c1 == '.')
            cnt = (rows - 2) / 2; // put dot at center
        if (c1 == '-')
            cnt = rows - 2;            // put dot at bottom
        screenbegin = begin_line(dot); // start dot at top
        dot_scroll(cnt, -1);
        break;
    case '|':                               // |- move to column "cmdcnt"
        dot = move_to_col(dot, cmdcnt - 1); // try to move to column
        break;
    case '~': // ~- flip the case of letters   a-z -> A-Z
        do {
            if (isalpha(*dot)) {
                undo_push(dot, 1, undo_del);
                *dot = islower(*dot) ? toupper(*dot) : tolower(*dot);
                undo_push(dot, 1, UNDO_INS_CHAIN);
                undo_del = UNDO_DEL_CHAIN;
            }
            dot_right();
        } while (--cmdcnt > 0);
        end_cmd_q(); // stop adding to q
        break;
        //----- The Cursor and Function Keys -----------------------------
    case KEYCODE_HOME: // Cursor Key Home
        dot_begin();
        break;
        // The Fn keys could point to do_macro which could translate them
    }

dc1:
    // if text[] just became empty, add back an empty line
    if (end == text) {
        char_insert(text, '\n', NO_UNDO); // start empty buf with dummy line
        dot = text;
    }
    // it is OK for dot to exactly equal to end, otherwise check dot validity
    if (dot != end) {
        dot = bound_dot(dot); // make sure "dot" is valid
    }
    if (dot != orig_dot)
        check_context(c); // update the current context

    if (!isdigit(c))
        cmdcnt = 0; // cmd was not a number, reset cmdcnt
    cnt = dot - begin_line(dot);
    // Try to stay off of the Newline
    if (*dot == '\n' && cnt > 0 && cmd_mode == 0)
        dot--;
}

