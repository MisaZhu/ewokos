#include <fnmatch.h>
#include <stddef.h>

static int fnm_char_eq(char pat, char str) {
    return pat == str;
}

/* Try to match a bracket expression; returns 1 on match and advances *pp. */
static int fnm_bracket(const char **pp, char c, int pathname, int period) {
    const char *p = *pp + 1; /* skip '[' */
    int negate = 0;
    int matched = 0;

    if (pathname && c == '/')
        return 0;
    if (period && c == '.')
        return 0;

    if (*p == '!' || *p == '^') {
        negate = 1;
        p++;
    }

    while (*p != '\0' && *p != ']') {
        char lo = *p++;
        if (*p == '-' && p[1] != '\0' && p[1] != ']') {
            char hi = p[1];
            p += 2;
            if (c >= lo && c <= hi)
                matched = 1;
        } else {
            if (c == lo)
                matched = 1;
        }
    }

    if (*p != ']')
        return 0; /* malformed: no closing bracket */

    *pp = p; /* leave pointing at ']' */
    return negate ? !matched : matched;
}

static int fnm_match(const char *pattern, const char *string, int flags,
        int leading) {
    int pathname = (flags & FNM_PATHNAME) != 0;
    int noescape = (flags & FNM_NOESCAPE) != 0;
    int period = (flags & FNM_PERIOD) != 0;

    while (*pattern != '\0') {
        char pc = *pattern;

        switch (pc) {
        case '\\':
            if (!noescape && pattern[1] != '\0') {
                pattern++;
                if (*string != *pattern)
                    return FNM_NOMATCH;
                if (pathname && *string == '/')
                    return FNM_NOMATCH;
                string++;
                pattern++;
                leading = 0;
                continue;
            }
            /* fall through: match literal backslash */
            if (*string != '\\')
                return FNM_NOMATCH;
            string++;
            pattern++;
            leading = 0;
            continue;
        case '?':
            if (*string == '\0')
                return FNM_NOMATCH;
            if (pathname && *string == '/')
                return FNM_NOMATCH;
            if (period && leading && *string == '.')
                return FNM_NOMATCH;
            string++;
            pattern++;
            leading = 0;
            continue;
        case '*': {
            /* collapse consecutive stars */
            while (*pattern == '*')
                pattern++;
            if (*pattern == '\0') {
                /* trailing star: matches unless it must span '/' */
                if (pathname) {
                    for (const char *q = string; *q != '\0'; q++) {
                        if (*q == '/')
                            return FNM_NOMATCH;
                    }
                }
                return 0;
            }
            while (*string != '\0') {
                if (fnm_match(pattern, string, flags, 0) == 0)
                    return 0;
                if (pathname && *string == '/')
                    return FNM_NOMATCH;
                string++;
            }
            return fnm_match(pattern, string, flags, 0);
        }
        case '[':
            if (*string == '\0')
                return FNM_NOMATCH;
            if (!fnm_bracket(&pattern, *string, pathname,
                    period && leading))
                return FNM_NOMATCH;
            string++;
            pattern++;
            leading = 0;
            continue;
        default:
            if (!fnm_char_eq(pc, *string))
                return FNM_NOMATCH;
            if (pathname && pc == '/')
                leading = 1;
            else
                leading = 0;
            string++;
            pattern++;
            continue;
        }
    }

    return (*string == '\0') ? 0 : FNM_NOMATCH;
}

int fnmatch(const char *pattern, const char *string, int flags) {
    if (pattern == NULL || string == NULL)
        return FNM_NOMATCH;
    return fnm_match(pattern, string, flags, 1);
}
