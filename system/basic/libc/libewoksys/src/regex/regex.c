/*
 * regex.c - POSIX 1003.2 regular expressions for EwokOS.
 *
 * A self-contained backtracking engine: regcomp() translates a BRE or ERE
 * pattern into a small instruction program, regexec() runs it with an
 * explicit (heap) backtrack stack so the C stack never grows with the input.
 *
 * Program model (each re_insn is one step):
 *   CHAR c      match one byte (case-folded when REG_ICASE)
 *   ANY         '.'  - one byte except NUL (and except '\n' with REG_NEWLINE)
 *   CLASS i     one byte in bracket-expression table entry i
 *   BOL/EOL     '^' / '$' anchors
 *   BOW/EOW     '\<' / '\>' word edges (GNU extension)
 *   WORDB/NWB   '\b' / '\B' word boundary (GNU extension)
 *   BACKREF n   repeat the text captured by group n
 *   SAVE k      record the cursor into capture slot k
 *   SPLIT x,y   try x first, remembering y as a backtrack point
 *   JMP x       unconditional jump
 *   MATCH       accept
 *
 * Repetition and alternation are built from SPLIT/JMP so the matcher stays a
 * single loop. Captures use slot 2n (start) and 2n+1 (end); group 0 is the
 * whole match.
 */
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* instruction set                                                     */
/* ------------------------------------------------------------------ */
enum {
	OP_CHAR, OP_ANY, OP_CLASS,
	OP_BOL, OP_EOL, OP_BOW, OP_EOW, OP_WORDB, OP_NWORDB,
	OP_BACKREF, OP_SAVE, OP_SPLIT, OP_JMP, OP_MATCH
};

typedef struct {
	int op;
	int x;    /* char / class index / slot / group / jump target */
	int y;    /* SPLIT alternate target */
} re_insn;

/* 256-bit membership bitmap for one bracket expression */
typedef struct {
	unsigned char neg;
	unsigned char map[32];
} re_class;

/* ------------------------------------------------------------------ */
/* compiler state                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
	const char *pat;
	int pos, len;
	int extended, icase, newline;

	re_insn  *prog; int nprog, cprog;
	re_class *cls;  int ncls,  ccls;

	int nsub;         /* capturing groups seen */
	int err;          /* REG_* error, 0 while healthy */
	int branch_first; /* next atom begins a branch (for '^') */
} re_comp;

/* Work limits: bound pathological backtracking so a bad pattern degrades to
 * "no match" instead of hanging or exhausting memory. Real patterns are far
 * below these. */
#define RE_STEP_LIMIT  (1L << 22)
#define RE_BT_LIMIT    (1L << 18)

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */
static int is_word_byte(int c) {
	return c >= 0 && c < 256 && (isalnum(c) || c == '_');
}

static void cls_set(re_class *cl, int ch) {
	unsigned c = (unsigned char)ch;
	cl->map[c >> 3] = (unsigned char)(cl->map[c >> 3] | (1u << (c & 7)));
}

static int cls_test(const re_class *cl, int ch) {
	unsigned c = (unsigned char)ch;
	int bit = (cl->map[c >> 3] >> (c & 7)) & 1;
	return cl->neg ? !bit : bit;
}

static int cc_cur(re_comp *c)  { return c->pos < c->len ? (unsigned char)c->pat[c->pos] : -1; }
static int cc_at(re_comp *c, int i) { return i >= 0 && i < c->len ? (unsigned char)c->pat[i] : -1; }

/* ------------------------------------------------------------------ */
/* program growth                                                      */
/* ------------------------------------------------------------------ */
static int prog_grow(re_comp *c) {
	int nc = c->cprog ? c->cprog * 2 : 64;
	re_insn *np = (re_insn *)realloc(c->prog, (size_t)nc * sizeof(re_insn));
	if (np == NULL) { c->err = REG_ESPACE; return -1; }
	c->prog = np; c->cprog = nc;
	return 0;
}

static int rc_emit(re_comp *c, int op, int x, int y) {
	if (c->err) return -1;
	if (c->nprog >= c->cprog && prog_grow(c) != 0) return -1;
	c->prog[c->nprog].op = op;
	c->prog[c->nprog].x = x;
	c->prog[c->nprog].y = y;
	return c->nprog++;
}

static int rc_new_class(re_comp *c) {
	if (c->err) return -1;
	if (c->ncls >= c->ccls) {
		int nc = c->ccls ? c->ccls * 2 : 8;
		re_class *np = (re_class *)realloc(c->cls, (size_t)nc * sizeof(re_class));
		if (np == NULL) { c->err = REG_ESPACE; return -1; }
		c->cls = np; c->ccls = nc;
	}
	memset(&c->cls[c->ncls], 0, sizeof(re_class));
	return c->ncls++;
}

/* Insert a SPLIT placeholder at index 'at', shifting the tail up by one and
 * repairing every jump target. Targets equal to 'at' keep pointing at 'at'
 * (now the SPLIT, i.e. the construct entry); targets past it shift by one. */
static int rc_insert_split(re_comp *c, int at) {
	if (c->err) return -1;
	if (c->nprog >= c->cprog && prog_grow(c) != 0) return -1;
	for (int i = c->nprog; i > at; i--)
		c->prog[i] = c->prog[i - 1];
	c->nprog++;
	c->prog[at].op = OP_SPLIT;
	c->prog[at].x = -1;
	c->prog[at].y = -1;
	for (int i = 0; i < c->nprog; i++) {
		if (i == at) continue;
		if (c->prog[i].op == OP_JMP) {
			if (c->prog[i].x > at) c->prog[i].x++;
		} else if (c->prog[i].op == OP_SPLIT) {
			if (c->prog[i].x > at) c->prog[i].x++;
			if (c->prog[i].y > at) c->prog[i].y++;
		}
	}
	return at;
}

/* ------------------------------------------------------------------ */
/* "can this instruction range match the empty string?"                */
/* Used to stop degenerate loops such as ()* or (a|)* from spinning.   */
/* ------------------------------------------------------------------ */
static int rc_empty_rec(re_comp *c, int pc, int end, unsigned char *vis) {
	while (pc < end) {
		if (pc < 0 || pc >= c->nprog) return 0;
		if (vis[pc]) return 0;
		vis[pc] = 1;
		re_insn *in = &c->prog[pc];
		switch (in->op) {
		case OP_CHAR: case OP_ANY: case OP_CLASS:
		case OP_BACKREF: case OP_MATCH:
			return 0;                       /* consumes input */
		case OP_JMP:
			if (in->x >= end) return 1;     /* jumps past the body */
			pc = in->x;
			continue;
		case OP_SPLIT:
			if (in->x >= end) return 1;
			if (rc_empty_rec(c, in->x, end, vis)) return 1;
			if (in->y >= end) return 1;
			return rc_empty_rec(c, in->y, end, vis);
		default:
			pc++;                           /* zero-width */
			continue;
		}
	}
	return 1;                                   /* fell through to end */
}

static int rc_can_empty(re_comp *c, int start, int end) {
	if (start >= end) return 1;
	unsigned char *vis = (unsigned char *)calloc((size_t)c->cprog, 1);
	if (vis == NULL) return 0;
	int r = rc_empty_rec(c, start, end, vis);
	free(vis);
	return r;
}

/* ------------------------------------------------------------------ */
/* character emission                                                  */
/* ------------------------------------------------------------------ */
static void rc_emit_char(re_comp *c, int ch) {
	rc_emit(c, OP_CHAR, (unsigned char)ch, 0);
}

static void rc_emit_shortcut(re_comp *c, int kind, int neg) {
	int idx = rc_new_class(c);
	if (idx < 0) return;
	re_class *cl = &c->cls[idx];
	cl->neg = (unsigned char)(neg ? 1 : 0);
	for (int ch = 0; ch < 256; ch++) {
		int m = 0;
		switch (kind) {
		case 'w': m = isalnum(ch) || ch == '_'; break;
		case 's': m = isspace(ch); break;
		case 'd': m = isdigit(ch); break;
		}
		if (m) cls_set(cl, ch);
	}
	rc_emit(c, OP_CLASS, idx, 0);
}

/* ------------------------------------------------------------------ */
/* bracket expressions                                                 */
/* ------------------------------------------------------------------ */
static void rc_class_one(re_class *cl, int ch, int icase) {
	cls_set(cl, ch);
	if (icase) {
		if (isupper(ch)) cls_set(cl, tolower(ch));
		else if (islower(ch)) cls_set(cl, toupper(ch));
	}
}

static void rc_class_range(re_class *cl, int lo, int hi, int icase) {
	for (int ch = lo; ch <= hi; ch++)
		rc_class_one(cl, ch, icase);
}

static int rc_apply_class_name(re_class *cl, const char *name, int nlen, int icase) {
	static const char *const names[] = {
		"alpha","digit","alnum","upper","lower","space",
		"blank","cntrl","print","graph","punct","xdigit","word"
	};
	int which = -1;
	for (int i = 0; i < (int)(sizeof(names)/sizeof(names[0])); i++) {
		if ((int)strlen(names[i]) == nlen && strncmp(names[i], name, (size_t)nlen) == 0) {
			which = i; break;
		}
	}
	if (which < 0) return 0;
	for (int ch = 0; ch < 256; ch++) {
		int m = 0;
		switch (which) {
		case 0: m = isalpha(ch); break;
		case 1: m = isdigit(ch); break;
		case 2: m = isalnum(ch); break;
		case 3: m = isupper(ch); break;
		case 4: m = islower(ch); break;
		case 5: m = isspace(ch); break;
		case 6: m = isblank(ch); break;
		case 7: m = iscntrl(ch); break;
		case 8: m = isprint(ch); break;
		case 9: m = isgraph(ch); break;
		case 10: m = ispunct(ch); break;
		case 11: m = isxdigit(ch); break;
		case 12: m = isalnum(ch) || ch == '_'; break;
		}
		if (m) rc_class_one(cl, ch, icase);
	}
	return 1;
}

/* Parse "[:name:]", "[.x.]" or "[=x=]" starting at c->pos (which points at
 * the inner '['). Returns 1 if a construct was consumed, 0 to fall back to a
 * literal '['. Sets c->err on a malformed/unknown construct. */
static int rc_parse_bracket_item(re_comp *c, re_class *cl) {
	int open = cc_at(c, c->pos + 1);
	int is_class = (open == ':');
	int is_coll  = (open == '.');
	int is_equiv = (open == '=');
	if (!is_class && !is_coll && !is_equiv) return 0;

	char closer = is_class ? ':' : (is_coll ? '.' : '=');
	int p = c->pos + 2;
	int start = p;
	while (p < c->len && !(c->pat[p] == closer && p + 1 < c->len && c->pat[p + 1] == ']'))
		p++;
	if (p >= c->len) { c->err = is_class ? REG_ECTYPE : REG_ECOLLATE; return 1; }

	if (is_class) {
		if (!rc_apply_class_name(cl, c->pat + start, p - start, c->icase))
			c->err = REG_ECTYPE;
	} else {
		/* collating element / equivalence class: treat the bytes literally */
		for (int i = start; i < p; i++)
			rc_class_one(cl, (unsigned char)c->pat[i], c->icase);
	}
	c->pos = p + 2;
	return 1;
}

/* c->pos points just past the opening '['. */
static void parse_class_expr(re_comp *c) {
	int idx = rc_new_class(c);
	if (idx < 0) return;
	re_class *cl = &c->cls[idx];
	cl->neg = 0;

	if (cc_cur(c) == '^') { cl->neg = 1; c->pos++; }

	int first = 1;
	for (;;) {
		int ch = cc_cur(c);
		if (ch < 0) { c->err = REG_EBRACK; return; }
		if (ch == ']' && !first) { c->pos++; break; }
		first = 0;

		if (ch == '[' && (cc_at(c, c->pos+1) == ':' ||
		                  cc_at(c, c->pos+1) == '.' ||
		                  cc_at(c, c->pos+1) == '=')) {
			if (rc_parse_bracket_item(c, cl)) {
				if (c->err) return;
				continue;
			}
		}

		int lo = ch;
		c->pos++;
		/* a '-' that is not final forms a range */
		if (cc_cur(c) == '-' && cc_at(c, c->pos+1) >= 0 && cc_at(c, c->pos+1) != ']') {
			c->pos++;
			int hi = cc_cur(c);
			if (hi < 0) { c->err = REG_EBRACK; return; }
			c->pos++;
			if (hi < lo) { c->err = REG_ERANGE; return; }
			rc_class_range(cl, lo, hi, c->icase);
		} else {
			rc_class_one(cl, lo, c->icase);
		}
	}

	/* With REG_NEWLINE a negated bracket must not match '\n' either. */
	if (cl->neg && c->newline)
		cls_set(cl, '\n');

	rc_emit(c, OP_CLASS, idx, 0);
}

/* ------------------------------------------------------------------ */
/* escapes                                                             */
/* ------------------------------------------------------------------ */
static void parse_group(re_comp *c);

/* e is the character following a backslash; c->pos is already past it. */
static void parse_escape(re_comp *c, int e) {
	if (!c->extended && e == '(') { parse_group(c); return; }
	if (e >= '1' && e <= '9') { rc_emit(c, OP_BACKREF, e - '0', 0); return; }

	switch (e) {
	case '<': rc_emit(c, OP_BOW, 0, 0); return;
	case '>': rc_emit(c, OP_EOW, 0, 0); return;
	case 'b': rc_emit(c, OP_WORDB, 0, 0); return;
	case 'B': rc_emit(c, OP_NWORDB, 0, 0); return;
	case 'w': rc_emit_shortcut(c, 'w', 0); return;
	case 'W': rc_emit_shortcut(c, 'w', 1); return;
	case 's': rc_emit_shortcut(c, 's', 0); return;
	case 'S': rc_emit_shortcut(c, 's', 1); return;
	case 'd': rc_emit_shortcut(c, 'd', 0); return;
	case 'D': rc_emit_shortcut(c, 'd', 1); return;
	case 'n': rc_emit_char(c, '\n'); return;
	case 't': rc_emit_char(c, '\t'); return;
	case 'r': rc_emit_char(c, '\r'); return;
	case 'f': rc_emit_char(c, '\f'); return;
	case 'v': rc_emit_char(c, '\v'); return;
	case 'a': rc_emit_char(c, '\a'); return;
	case 'e': rc_emit_char(c, 0x1b); return;
	default:  rc_emit_char(c, e); return;   /* literal (covers \. \* \\ \) ...) */
	}
}

/* ------------------------------------------------------------------ */
/* branch/position predicates                                          */
/* ------------------------------------------------------------------ */
static int at_alt_sep(re_comp *c) {
	if (c->extended) return cc_cur(c) == '|';
	return cc_cur(c) == '\\' && cc_at(c, c->pos+1) == '|';
}
static void eat_alt_sep(re_comp *c) { c->pos += c->extended ? 1 : 2; }

static int at_group_close(re_comp *c) {
	if (c->extended) return cc_cur(c) == ')';
	return cc_cur(c) == '\\' && cc_at(c, c->pos+1) == ')';
}
static int eat_group_close(re_comp *c) {
	if (!at_group_close(c)) return 0;
	c->pos += c->extended ? 1 : 2;
	return 1;
}
static int at_branch_end(re_comp *c) {
	return c->pos >= c->len || at_alt_sep(c) || at_group_close(c);
}

/* ------------------------------------------------------------------ */
/* interval {m,n}                                                      */
/* ------------------------------------------------------------------ */
static int read_count(re_comp *c, int *out) {
	int p = c->pos, v = 0, n = 0;
	while (p < c->len && c->pat[p] >= '0' && c->pat[p] <= '9') {
		v = v * 10 + (c->pat[p] - '0');
		if (v > 65535) return -1;      /* absurd repeat */
		p++; n++;
	}
	if (n == 0) return 0;
	c->pos = p; *out = v;
	return 1;
}

/* c->pos is just past '{' (ERE) or '\{' (BRE). On success consumes through
 * the closing '}' / '\}' and returns 1 with lo and hi filled (hi==-1 means
 * unbounded). On failure restores c->pos and returns 0. */
static int parse_interval(re_comp *c, int *lo, int *hi) {
	int save = c->pos;
	int m = -1, n = -1;

	int rm = read_count(c, &m);
	if (rm < 0) { c->err = REG_BADBR; return 1; }
	if (rm == 0) { c->pos = save; return 0; }
	if (cc_cur(c) == ',') {
		c->pos++;
		int r = read_count(c, &n);
		if (r < 0) { c->err = REG_BADBR; return 1; }
		if (r == 0) n = -1;            /* {m,} unbounded */
	} else {
		n = m;
	}

	if (c->extended) {
		if (cc_cur(c) != '}') { c->pos = save; return 0; }
		c->pos++;
	} else {
		if (!(cc_cur(c) == '\\' && cc_at(c, c->pos+1) == '}')) { c->pos = save; return 0; }
		c->pos += 2;
	}

	if (n >= 0 && n < m) { c->err = REG_BADBR; return 1; }
	*lo = m; *hi = n;
	return 1;
}

/* Does a quantifier start at c->pos? If so consume it and return its kind
 * ('*','+','?','{'), filling lo/hi for '{'. */
static int peek_quant(re_comp *c, int *lo, int *hi) {
	if (c->pos >= c->len) return 0;
	int ch = cc_cur(c);

	if (c->extended) {
		if (ch == '*' || ch == '+' || ch == '?') { c->pos++; return ch; }
		if (ch == '{') {
			int save = c->pos;
			c->pos++;                 /* past '{' */
			int m, n;
			if (parse_interval(c, &m, &n)) {
				if (c->err) return '{';
				*lo = m; *hi = n; return '{';
			}
			c->pos = save;            /* not an interval -> literal '{' */
			return 0;
		}
		return 0;
	}

	/* BRE: '*' is a quantifier; GNU adds \+ \? and \{m,n\} */
	if (ch == '*') { c->pos++; return '*'; }
	if (ch == '\\' && c->pos + 1 < c->len) {
		int e = cc_at(c, c->pos+1);
		if (e == '+') { c->pos += 2; return '+'; }
		if (e == '?') { c->pos += 2; return '?'; }
		if (e == '{') {
			int save = c->pos;
			c->pos += 2;              /* past '\{' */
			int m, n;
			if (parse_interval(c, &m, &n)) {
				if (c->err) return '{';
				*lo = m; *hi = n; return '{';
			}
			c->pos = save;
			return 0;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* repetition construction                                             */
/* ------------------------------------------------------------------ */
/* body already emitted at [atom_start, c->nprog) */
static void rc_star(re_comp *c, int atom_start) {
	int split = rc_insert_split(c, atom_start);
	if (split < 0) return;
	int body = atom_start + 1;
	if (rc_can_empty(c, body, c->nprog)) {
		/* degenerate: one optional pass, no back edge */
		c->prog[split].x = body;
		c->prog[split].y = c->nprog;
		return;
	}
	int jmp = rc_emit(c, OP_JMP, split, 0);
	c->prog[split].x = body;              /* greedy: prefer the body */
	c->prog[split].y = jmp + 1;           /* exit */
}

static void rc_plus(re_comp *c, int atom_start) {
	int body_end = c->nprog;
	if (rc_can_empty(c, atom_start, body_end))
		return;                         /* e+ with empty-able e == e once */
	int split = rc_emit(c, OP_SPLIT, atom_start, c->nprog + 1);
	c->prog[split].x = atom_start;        /* loop back (greedy) */
	c->prog[split].y = c->nprog;          /* exit == split+1 */
}

static void rc_opt(re_comp *c, int atom_start) {
	int split = rc_insert_split(c, atom_start);
	if (split < 0) return;
	c->prog[split].x = atom_start + 1;    /* prefer the body */
	c->prog[split].y = c->nprog;          /* skip it */
}

/* Append a verbatim copy of body[0..body_len) whose original home was
 * atom_start, rewriting jump targets for the new location. */
static void rc_copy_body(re_comp *c, const re_insn *body, int body_len, int atom_start) {
	int dest = c->nprog;
	int delta = dest - atom_start;
	for (int i = 0; i < body_len && !c->err; i++) {
		re_insn in = body[i];
		if (in.op == OP_JMP) in.x += delta;
		else if (in.op == OP_SPLIT) { in.x += delta; in.y += delta; }
		rc_emit(c, in.op, in.x, in.y);
	}
}

/* e{lo,hi}: expand by duplication (bounded), so it can never spin. */
static void rc_interval(re_comp *c, int atom_start, int lo, int hi) {
	int body_len = c->nprog - atom_start;
	int emptyable = rc_can_empty(c, atom_start, c->nprog);

	re_insn *body = (re_insn *)malloc((size_t)body_len * sizeof(re_insn));
	if (body == NULL) { c->err = REG_ESPACE; return; }
	memcpy(body, &c->prog[atom_start], (size_t)body_len * sizeof(re_insn));
	c->nprog = atom_start;               /* rewind; re-emit expanded form */

	for (int i = 0; i < lo && !c->err; i++)
		rc_copy_body(c, body, body_len, atom_start);

	if (hi < 0) {
		/* e{lo,} = e^lo e*  (a single star loop; guard the empty case) */
		if (emptyable) {
			rc_copy_body(c, body, body_len, atom_start);
		} else {
			int loop_start = c->nprog;
			rc_copy_body(c, body, body_len, atom_start);
			int split = rc_emit(c, OP_SPLIT, loop_start, c->nprog + 1);
			c->prog[split].x = loop_start;
			c->prog[split].y = c->nprog;
		}
	} else {
		for (int i = lo; i < hi && !c->err; i++) {
			int split = rc_emit(c, OP_SPLIT, -1, -1);
			int body_at = c->nprog;
			rc_copy_body(c, body, body_len, atom_start);
			c->prog[split].x = body_at;      /* prefer this copy */
			c->prog[split].y = c->nprog;     /* or skip it */
		}
	}
	free(body);
}

/* ------------------------------------------------------------------ */
/* recursive-descent parser                                            */
/* ------------------------------------------------------------------ */
static void parse_alt(re_comp *c);
static void parse_branch(re_comp *c);
static void parse_piece(re_comp *c);
static void parse_atom(re_comp *c);

static void parse_group(re_comp *c) {
	int capturing = 1;
	/* (?:...) non-capturing group (GNU extension) */
	if (cc_cur(c) == '?' && cc_at(c, c->pos+1) == ':') {
		c->pos += 2;
		capturing = 0;
	}
	int g = 0;
	if (capturing) {
		g = ++c->nsub;
		if (g > 65535) { c->err = REG_EPAREN; return; }
		rc_emit(c, OP_SAVE, 2 * g, 0);
	}
	parse_alt(c);
	if (c->err) return;
	if (!eat_group_close(c)) { c->err = REG_EPAREN; return; }
	if (capturing) rc_emit(c, OP_SAVE, 2 * g + 1, 0);
}

static void parse_atom(re_comp *c) {
	int ch = cc_cur(c);
	if (ch < 0) return;
	c->pos++;

	switch (ch) {
	case '\\': {
		int e = cc_cur(c);
		if (e < 0) { c->err = REG_EESCAPE; return; }
		c->pos++;
		parse_escape(c, e);
		return;
	}
	case '.':
		rc_emit(c, OP_ANY, 0, 0);
		return;
	case '[':
		parse_class_expr(c);
		return;
	case '^':
		if (c->branch_first) rc_emit(c, OP_BOL, 0, 0);
		else rc_emit_char(c, '^');
		return;
	case '$':
		if (at_branch_end(c)) rc_emit(c, OP_EOL, 0, 0);
		else rc_emit_char(c, '$');
		return;
	case '(':
		if (c->extended) { parse_group(c); return; }
		rc_emit_char(c, '(');
		return;
	default:
		/* '*', '+', '?', '{', '|', ')' reaching an atom position have
		 * nothing to repeat/alternate, so they are ordinary literals. */
		rc_emit_char(c, ch);
		return;
	}
}

static void parse_piece(re_comp *c) {
	int atom_start = c->nprog;
	parse_atom(c);
	if (c->err) return;
	int atom_end = c->nprog;

	int lo = 0, hi = 0;
	int kind = peek_quant(c, &lo, &hi);
	if (c->err) return;
	if (kind == 0) return;
	if (atom_end == atom_start) return;   /* nothing emitted to repeat */

	switch (kind) {
	case '*': rc_star(c, atom_start); break;
	case '+': rc_plus(c, atom_start); break;
	case '?': rc_opt(c, atom_start); break;
	case '{': rc_interval(c, atom_start, lo, hi); break;
	}
}

static void parse_branch(re_comp *c) {
	c->branch_first = 1;
	while (!c->err && c->pos < c->len) {
		if (at_alt_sep(c)) break;
		if (at_group_close(c)) break;
		parse_piece(c);
		c->branch_first = 0;
	}
}

static void parse_alt(re_comp *c) {
	int split_pc = rc_emit(c, OP_SPLIT, -1, -1);
	if (split_pc < 0) return;
	int body_pc = c->nprog;

	parse_branch(c);
	if (c->err) return;

	if (at_alt_sep(c)) {
		eat_alt_sep(c);
		int jmp_pc = rc_emit(c, OP_JMP, -1, 0);
		c->prog[split_pc].x = body_pc;      /* prefer this alternative */
		c->prog[split_pc].y = c->nprog;     /* else the rest */
		parse_alt(c);
		if (c->err) return;
		c->prog[jmp_pc].x = c->nprog;       /* jump past all alternatives */
	} else {
		/* not an alternation: turn the reserved SPLIT into a no-op hop */
		c->prog[split_pc].op = OP_JMP;
		c->prog[split_pc].x = body_pc;      /* == split_pc + 1 */
		c->prog[split_pc].y = 0;
	}
}


/* ------------------------------------------------------------------ */
/* matcher                                                             */
/* ------------------------------------------------------------------ */
typedef struct { int pc; const char *sp; int logdepth; } re_bt;
typedef struct { int slot; const char *old; } re_sv;

typedef struct {
	const char **slots; int nslots;
	re_bt *bt; int nbt, cbt;
	re_sv *log; int nlog, clog;
} re_scratch;

static int sc_init(re_scratch *s, int nslots) {
	memset(s, 0, sizeof(*s));
	s->nslots = nslots;
	s->slots = (const char **)malloc((size_t)nslots * sizeof(const char *));
	s->cbt = 256;
	s->bt = (re_bt *)malloc((size_t)s->cbt * sizeof(re_bt));
	s->clog = 256;
	s->log = (re_sv *)malloc((size_t)s->clog * sizeof(re_sv));
	if (s->slots == NULL || s->bt == NULL || s->log == NULL) {
		free(s->slots); free(s->bt); free(s->log);
		s->slots = NULL; s->bt = NULL; s->log = NULL;
		return -1;
	}
	return 0;
}

static void sc_free(re_scratch *s) {
	free(s->slots); free(s->bt); free(s->log);
	s->slots = NULL; s->bt = NULL; s->log = NULL;
}

/* Remember an alternate branch. When the backtrack stack is exhausted by a
 * pathological pattern the alternative is simply dropped, degrading to
 * "no match" rather than growing without bound. */
static void sc_push_bt(re_scratch *s, int pc, const char *sp) {
	if (s->nbt >= RE_BT_LIMIT) return;
	if (s->nbt == s->cbt) {
		int nc = s->cbt * 2;
		if (nc > RE_BT_LIMIT) nc = (int)RE_BT_LIMIT;
		re_bt *np = (re_bt *)realloc(s->bt, (size_t)nc * sizeof(re_bt));
		if (np == NULL) return;
		s->bt = np; s->cbt = nc;
	}
	s->bt[s->nbt].pc = pc;
	s->bt[s->nbt].sp = sp;
	s->bt[s->nbt].logdepth = s->nlog;
	s->nbt++;
}

static int sc_push_log(re_scratch *s, int slot, const char *old) {
	if (s->nlog == s->clog) {
		int nc = s->clog * 2;
		re_sv *np = (re_sv *)realloc(s->log, (size_t)nc * sizeof(re_sv));
		if (np == NULL) return -1;
		s->log = np; s->clog = nc;
	}
	s->log[s->nlog].slot = slot;
	s->log[s->nlog].old = old;
	s->nlog++;
	return 0;
}

/* Run the program against `base` starting at `start`. Returns 1 on match
 * (filling pmatch unless REG_NOSUB), 0 if no match from this start. */
static int re_run(const regex_t *preg, const char *base, const char *start,
                  size_t nmatch, regmatch_t pmatch[], int eflags, re_scratch *s) {
	const re_insn *prog = (const re_insn *)preg->re_prog;
	const re_class *cls = (const re_class *)preg->re_classes;
	int icase   = (preg->re_cflags & REG_ICASE) ? 1 : 0;
	int newline = (preg->re_cflags & REG_NEWLINE) ? 1 : 0;
	int nosub   = (preg->re_cflags & REG_NOSUB) ? 1 : 0;

	for (int i = 0; i < s->nslots; i++) s->slots[i] = NULL;
	s->nbt = 0;
	s->nlog = 0;

	int pc = 0;
	const char *sp = start;
	long steps = 0;

	for (;;) {
		const re_insn *in = &prog[pc];
		if (++steps > RE_STEP_LIMIT) goto backtrack;
		switch (in->op) {
		case OP_CHAR:
			if (*sp != '\0' &&
			    ((unsigned char)*sp == (unsigned char)in->x ||
			     (icase && tolower((unsigned char)*sp) == tolower((unsigned char)in->x)))) {
				sp++; pc++; continue;
			}
			goto backtrack;
		case OP_ANY:
			if (*sp != '\0' && !(newline && *sp == '\n')) { sp++; pc++; continue; }
			goto backtrack;
		case OP_CLASS:
			if (*sp != '\0' && cls_test(&cls[in->x], (unsigned char)*sp)) { sp++; pc++; continue; }
			goto backtrack;
		case OP_BOL:
			if ((sp == base && !(eflags & REG_NOTBOL)) ||
			    (newline && sp > base && sp[-1] == '\n')) { pc++; continue; }
			goto backtrack;
		case OP_EOL:
			if ((*sp == '\0' && !(eflags & REG_NOTEOL)) ||
			    (newline && *sp == '\n')) { pc++; continue; }
			goto backtrack;
		case OP_BOW:
			if (!((sp > base) && is_word_byte((unsigned char)sp[-1])) &&
			    is_word_byte((unsigned char)*sp)) { pc++; continue; }
			goto backtrack;
		case OP_EOW:
			if (((sp > base) && is_word_byte((unsigned char)sp[-1])) &&
			    !is_word_byte((unsigned char)*sp)) { pc++; continue; }
			goto backtrack;
		case OP_WORDB: {
			int pw = (sp > base) && is_word_byte((unsigned char)sp[-1]);
			int cw = is_word_byte((unsigned char)*sp);
			if (pw != cw) { pc++; continue; }
			goto backtrack;
		}
		case OP_NWORDB: {
			int pw = (sp > base) && is_word_byte((unsigned char)sp[-1]);
			int cw = is_word_byte((unsigned char)*sp);
			if (pw == cw) { pc++; continue; }
			goto backtrack;
		}
		case OP_BACKREF: {
			int g = in->x;
			const char *gs = s->slots[2*g];
			const char *ge = s->slots[2*g+1];
			if (gs == NULL || ge == NULL) { pc++; continue; }  /* unset: matches empty */
			size_t glen = (size_t)(ge - gs);
			size_t i = 0;
			for (; i < glen; i++) {
				unsigned char a = (unsigned char)sp[i];
				unsigned char b = (unsigned char)gs[i];
				if (a == '\0') break;
				if (icase ? (tolower(a) != tolower(b)) : (a != b)) break;
			}
			if (i == glen) { sp += glen; pc++; continue; }
			goto backtrack;
		}
		case OP_SAVE:
			if (sc_push_log(s, in->x, s->slots[in->x]) != 0) goto backtrack;
			s->slots[in->x] = sp;
			pc++;
			continue;
		case OP_JMP:
			pc = in->x;
			continue;
		case OP_SPLIT:
			sc_push_bt(s, in->y, sp);
			pc = in->x;
			continue;
		case OP_MATCH:
			if (!nosub && pmatch != NULL) {
				size_t avail = preg->re_nsub + 1;
				for (size_t i = 0; i < nmatch; i++) {
					if (i < avail && s->slots[2*i] != NULL && s->slots[2*i+1] != NULL) {
						pmatch[i].rm_so = (regoff_t)(s->slots[2*i] - base);
						pmatch[i].rm_eo = (regoff_t)(s->slots[2*i+1] - base);
					} else {
						pmatch[i].rm_so = -1;
						pmatch[i].rm_eo = -1;
					}
				}
			}
			return 1;
		default:
			goto backtrack;
		}
	backtrack:
		if (s->nbt == 0) return 0;
		{
			re_bt bt = s->bt[--s->nbt];
			while (s->nlog > bt.logdepth) {
				s->nlog--;
				s->slots[s->log[s->nlog].slot] = s->log[s->nlog].old;
			}
			pc = bt.pc;
			sp = bt.sp;
		}
	}
}

/* True when the preferred path reaches OP_BOL before consuming anything, so
 * only the start of the subject (or of a line, with REG_NEWLINE) can match. */
static int prog_starts_bol(const regex_t *preg) {
	const re_insn *prog = (const re_insn *)preg->re_prog;
	int n = (int)preg->re_nprog;
	int pc = 0, guard = 0;
	while (pc >= 0 && pc < n && guard++ <= n) {
		switch (prog[pc].op) {
		case OP_BOL:  return 1;
		case OP_SAVE: pc++; continue;
		case OP_JMP:  pc = prog[pc].x; continue;
		case OP_SPLIT: pc = prog[pc].x; continue;
		default:      return 0;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* public API                                                          */
/* ------------------------------------------------------------------ */
int regcomp(regex_t *preg, const char *pattern, int cflags) {
	if (preg == NULL || pattern == NULL) return REG_BADPAT;
	memset(preg, 0, sizeof(*preg));
	preg->re_cflags = cflags;

	re_comp c;
	memset(&c, 0, sizeof(c));
	c.pat = pattern;
	c.len = (int)strlen(pattern);
	c.extended = (cflags & REG_EXTENDED) ? 1 : 0;
	c.icase    = (cflags & REG_ICASE) ? 1 : 0;
	c.newline  = (cflags & REG_NEWLINE) ? 1 : 0;

	rc_emit(&c, OP_SAVE, 0, 0);          /* group 0 start */
	c.branch_first = 1;
	parse_alt(&c);
	if (c.err == 0 && c.pos < c.len)
		c.err = REG_EPAREN;          /* stray ')' / '\)' */
	if (c.err == 0) {
		rc_emit(&c, OP_SAVE, 1, 0);  /* group 0 end */
		rc_emit(&c, OP_MATCH, 0, 0);
	}

	if (c.err != 0) {
		int e = c.err;
		free(c.prog);
		free(c.cls);
		preg->re_err = e;
		return e;
	}

	preg->re_nsub    = (size_t)c.nsub;
	preg->re_prog    = c.prog;
	preg->re_nprog   = (size_t)c.nprog;
	preg->re_classes = c.cls;
	preg->re_nclass  = (size_t)c.ncls;
	preg->re_err     = 0;
	return 0;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch,
            regmatch_t pmatch[], int eflags) {
	if (preg == NULL || preg->re_prog == NULL || string == NULL)
		return REG_NOMATCH;
	if (preg->re_err != 0)
		return preg->re_err;
	if ((preg->re_cflags & REG_NOSUB) || nmatch == 0 || pmatch == NULL)
		nmatch = 0;                 /* nothing to report */

	re_scratch s;
	if (sc_init(&s, 2 * ((int)preg->re_nsub + 1)) != 0)
		return REG_NOMATCH;         /* out of memory: report no match */

	int bol_anchored = prog_starts_bol(preg) && !(preg->re_cflags & REG_NEWLINE);

	int r = REG_NOMATCH;
	for (const char *p = string;; p++) {
		if (re_run(preg, string, p, nmatch, pmatch, eflags, &s)) {
			r = REG_NOERROR;
			break;
		}
		if (bol_anchored || *p == '\0')
			break;
	}
	sc_free(&s);
	return r;
}

size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size) {
	(void)preg;
	const char *msg;
	switch (errcode) {
	case REG_NOERROR:  msg = "Success"; break;
	case REG_NOMATCH:  msg = "No match"; break;
	case REG_BADPAT:   msg = "Invalid regular expression"; break;
	case REG_ECOLLATE: msg = "Invalid collating element"; break;
	case REG_ECTYPE:   msg = "Invalid character class name"; break;
	case REG_EESCAPE:  msg = "Trailing backslash"; break;
	case REG_ESUBREG:  msg = "Invalid back reference"; break;
	case REG_EBRACK:   msg = "Unmatched [ or [^"; break;
	case REG_EPAREN:   msg = "Unmatched ( or \\)"; break;
	case REG_EBRACE:   msg = "Unmatched \\{"; break;
	case REG_BADBR:    msg = "Invalid content of \\{\\}"; break;
	case REG_ERANGE:   msg = "Invalid range end"; break;
	case REG_ESPACE:   msg = "Out of memory"; break;
	case REG_BADRPT:   msg = "Repetition operator in invalid position"; break;
	default:           msg = "Unknown error"; break;
	}

	size_t need = strlen(msg) + 1;
	if (errbuf != NULL && errbuf_size > 0) {
		size_t copy = need < errbuf_size ? need : errbuf_size;
		memcpy(errbuf, msg, copy - 1);
		errbuf[copy - 1] = '\0';
	}
	return need;
}

void regfree(regex_t *preg) {
	if (preg == NULL) return;
	free(preg->re_prog);
	free(preg->re_classes);
	memset(preg, 0, sizeof(*preg));
}
