/* vim_syntax.h — syntax highlighting for .json, .rd (init script),
 * .conf (key=value), .js (JavaScript) and .c/.h (C source) files.
 *
 * The highlighter is hooked into refresh() (vim_term.c): whenever a screen
 * line changes, the visible slice is written through syntax_write_slice(),
 * which emits SGR color sequences around the tokens of the line.
 * Colors are computed per line so each line can be colored independently:
 * JSON strings may not contain raw newlines, .rd files are executed line by
 * line, .conf is parsed line by line by the kernel sconf library.  JS/C block
 * comments can span lines but are colored only on the line where they open,
 * which keeps colors a pure function of the line text (see vim_syntax.c).
 */
#pragma once

#include <stdint.h>

enum {
    SYNTAX_NONE = 0,
    SYNTAX_JSON,
    SYNTAX_RD,
    SYNTAX_CONF,
    SYNTAX_JS,
    SYNTAX_C
};

// select the highlight mode from the file name extension
// (.json / .rd / .conf / .js / .c / .h)
void syntax_set_file(const char* fn);

// the currently active highlight mode (one of SYNTAX_*)
int syntax_mode(void);

/* Write vline[from..to] (inclusive) to stdout, adding SGR color sequences
 * when a syntax mode is active.  vline is one formatted screen line as
 * produced by format_line(): exactly 'columns' chars, space padded,
 * tabs/control chars already expanded.  When sel is not NULL it carries a
 * per-column flag marking the visual selection, shown in reverse video
 * (sel overrides the token color for the marked columns). */
void syntax_write_slice(const char* vline, int from, int to, const uint8_t* sel);
