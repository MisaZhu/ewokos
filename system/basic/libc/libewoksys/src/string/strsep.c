#include <string.h>

char* strsep(char** stringp, const char* delim) {
    char* s = *stringp;
    char* p;
    if (s == NULL)
        return NULL;
    s += strspn(s, delim);
    p = strpbrk(s, delim);
    if (p != NULL) {
        *p = '\0';
        *stringp = p + 1;
    } else {
        *stringp = NULL;
    }
    return s;
}
