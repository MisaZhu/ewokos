/* mkdir - make directories.
   Command-line and diagnostic-format compatible with GNU coreutils mkdir.

   Options: -m/--mode, -p/--parents, -v/--verbose, -Z/--context (accepted and
   ignored; EwokOS has no SELinux), --help, --version and the "--" separator.
   Several DIRECTORY operands may be given; they are processed in order and the
   exit status is 0 only when every one of them succeeded.

   Modes follow GNU: the default is 0777 & ~umask; "-m" accepts an octal value
   or a symbolic clause whose point of departure is a=rwx; with "-p" the
   intermediate directories get the umask default OR'd with u+wx while only the
   final component receives the "-m" mode. Diagnostics mirror GNU wording, e.g.
   "mkdir: cannot create directory 'x': No such file or directory". */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ewoksys/vfs.h>

/* Long-only option sentinels; getopt_long returns val unchanged. */
enum {
    OPT_HELP = 256,
    OPT_VERSION
};

/* Symbolic-mode "who" classes. */
#define MODE_WHO_U 0x1
#define MODE_WHO_G 0x2
#define MODE_WHO_O 0x4
#define MODE_WHO_A (MODE_WHO_U | MODE_WHO_G | MODE_WHO_O)

static int opt_parents = 0;
static int opt_verbose = 0;
static int opt_mode_given = 0;
static mode_t opt_mode = 0;

static void usage(int status) {
    if(status != 0) {
        fputs("Try 'mkdir --help' for more information.\n", stderr);
        exit(status);
    }
    fputs(
"Usage: mkdir [OPTION]... DIRECTORY...\n"
"Create the DIRECTORY(ies), if they do not already exist.\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"  -m, --mode=MODE   set file mode (as in chmod), not a=rwx - umask\n"
"  -p, --parents     no error if existing, make parent directories as needed\n"
"  -v, --verbose     print a message for each created directory\n"
"  -Z, --context=CTX set the SELinux security context to CTX (ignored on EwokOS)\n"
"      --help        display this help and exit\n"
"      --version     output version information and exit\n", stdout);
    exit(0);
}

/* Parse a "-m" mode. Accepts either an octal value (1..4 digits) or a GNU
   symbolic clause list whose point of departure is a=rwx. Returns 0 on success
   and stores the resulting permission bits in *out, or -1 on invalid syntax. */
static int parse_mode(const char* s, mode_t* out) {
    if(s == NULL || *s == 0)
        return -1;

    /* Octal form: nothing but 1..4 octal digits. */
    {
        const char* p = s;
        int n = 0;
        while(*p >= '0' && *p <= '7') {
            p++;
            n++;
        }
        if(n >= 1 && n <= 4 && *p == 0) {
            *out = (mode_t)(strtoul(s, NULL, 8) & 07777);
            return 0;
        }
    }

    /* Symbolic form. */
    mode_t result = 0777; /* a=rwx point of departure, as GNU mkdir does */
    while(*s != 0) {
        int who = 0;
        while(*s == 'u' || *s == 'g' || *s == 'o' || *s == 'a') {
            if(*s == 'u')       who |= MODE_WHO_U;
            else if(*s == 'g')  who |= MODE_WHO_G;
            else if(*s == 'o')  who |= MODE_WHO_O;
            else                who  = MODE_WHO_A;
            s++;
        }
        if(who == 0)
            who = MODE_WHO_A; /* no explicit who => all classes */

        int did_op = 0;
        while(*s == '=' || *s == '+' || *s == '-') {
            char op = *s++;
            did_op = 1;

            /* Build a template of the requested bits in the user positions. */
            mode_t tpl = 0;
            int sp_suid = 0, sp_sgid = 0, sp_sticky = 0;
            for(;;) {
                char c = *s;
                if(c == 'r')                 { tpl |= S_IRUSR; s++; }
                else if(c == 'w')            { tpl |= S_IWUSR; s++; }
                else if(c == 'x' || c == 'X'){ tpl |= S_IXUSR; s++; } /* X==x for dirs */
                else if(c == 's')            { sp_suid = sp_sgid = 1; s++; }
                else if(c == 't')            { sp_sticky = 1; s++; }
                else if(c == 'u')            { tpl |= (result & S_IRWXU); s++; }
                else if(c == 'g')            { tpl |= ((result & S_IRWXG) << 3); s++; }
                else if(c == 'o')            { tpl |= ((result & S_IRWXO) << 6); s++; }
                else break;
            }

            /* Replicate the user-position template onto each selected class. */
            mode_t apply = 0;
            if(who & MODE_WHO_U) apply |= (tpl & S_IRWXU);
            if(who & MODE_WHO_G) apply |= ((tpl & S_IRWXU) >> 3);
            if(who & MODE_WHO_O) apply |= ((tpl & S_IRWXU) >> 6);
            if(sp_suid && (who & MODE_WHO_U)) apply |= S_ISUID;
            if(sp_sgid && (who & MODE_WHO_G)) apply |= S_ISGID;
            if(sp_sticky)                     apply |= S_ISVTX;

            if(op == '=') {
                mode_t clearmask = 0;
                if(who & MODE_WHO_U) clearmask |= S_IRWXU | S_ISUID;
                if(who & MODE_WHO_G) clearmask |= S_IRWXG | S_ISGID;
                if(who & MODE_WHO_O) clearmask |= S_IRWXO;
                result = (result & ~clearmask) | apply;
            } else if(op == '+') {
                result |= apply;
            } else { /* '-' */
                result &= ~apply;
            }
        }
        if(!did_op)
            return -1; /* a clause with a "who" but no operator, e.g. "u" */

        if(*s == ',') {
            s++;
            continue;
        }
        if(*s == 0)
            break;
        return -1; /* trailing garbage */
    }

    *out = result & 07777;
    return 0;
}

/* GNU-style capitalized reason for an errno value. */
static const char* err_reason(int e) {
    switch(e) {
    case EEXIST:       return "File exists";
    case ENOENT:       return "No such file or directory";
    case ENOTDIR:      return "Not a directory";
    case EISDIR:       return "Is a directory";
    case EACCES:
    case EPERM:        return "Permission denied";
    case EROFS:        return "Read-only file system";
    case ENOSPC:       return "No space left on device";
    case ENAMETOOLONG: return "File name too long";
    case EIO:          return "Input/output error";
    default:           return strerror(e);
    }
}

static int report_cannot(const char* disp, int err) {
    fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", disp, err_reason(err));
    return -1;
}

/* Copy 'arg' into 'out' with GNU-ish normalization: drop leading "./", collapse
   repeated '/' and strip trailing '/' (except for a bare "/"). Returns 0 on
   success or -1 when nothing usable remains (empty operand / too long). */
static int normalize_operand(const char* arg, char* out, size_t outsz) {
    while(arg[0] == '.' && arg[1] == '/') {
        arg += 2;
        while(*arg == '/')
            arg++;
    }

    size_t o = 0;
    int prev_slash = 0;
    for(const char* p = arg; *p != 0; p++) {
        if(*p == '/') {
            if(o == 0 || !prev_slash) {
                if(o + 1 >= outsz) return -1;
                out[o++] = '/';
            }
            prev_slash = 1;
        } else {
            if(o + 1 >= outsz) return -1;
            out[o++] = *p;
            prev_slash = 0;
        }
    }
    out[o] = 0;
    while(o > 1 && out[o - 1] == '/')
        out[--o] = 0;
    return (o == 0) ? -1 : 0;
}

/* Create the directory described by 'path' (already normalized, mutable).
   'disp' is the string used in diagnostics (the full target). Returns 0 on
   success, -1 on error. */
static int make_dir(char* path, const char* disp, mode_t final_mode, mode_t mid_mode) {
    if(!opt_parents) {
        /* POSIX mkdir: the parent must exist and the target must not. */
        struct stat st;
        if(stat(path, &st) == 0)
            return report_cannot(disp, EEXIST);
        errno = 0;
        /* autodir=false so a missing parent is NOT silently created. */
        if(vfs_create(path, NULL, FS_TYPE_DIR, final_mode, false, false) != 0) {
            int e = errno;
            return report_cannot(disp, e != 0 ? e : EIO);
        }
        if(opt_verbose)
            printf("mkdir: created directory '%s'\n", disp);
        return 0;
    }

    /* -p: create every missing component top-down. */
    size_t len = strlen(path);
    size_t i = (path[0] == '/') ? 1 : 0; /* the root always exists */
    while(i < len) {
        size_t j = i;
        while(j < len && path[j] != '/')
            j++;

        int is_final = (j == len);
        char save = path[j];
        path[j] = 0;

        int created = 0;
        int err = 0;
        if(mkdir(path, is_final ? final_mode : mid_mode) == 0) {
            created = 1;
        } else if(errno == EEXIST) {
            struct stat st;
            if(stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                err = 0;                 /* existing directory: -p ignores it */
            } else {
                err = is_final ? EEXIST : ENOTDIR;
            }
        } else {
            err = errno;
        }

        if(created && opt_verbose)
            printf("mkdir: created directory '%s'\n", path);

        path[j] = save;
        if(err != 0)
            return report_cannot(disp, err);

        i = j;
        while(i < len && path[i] == '/')
            i++;
    }
    return 0;
}

int main(int argc, char** argv) {
    static struct option longopts[] = {
        {"mode",    required_argument, NULL, 'm'},
        {"parents", no_argument,       NULL, 'p'},
        {"verbose", no_argument,       NULL, 'v'},
        {"context", required_argument, NULL, 'Z'},
        {"help",    no_argument,       NULL, OPT_HELP},
        {"version", no_argument,       NULL, OPT_VERSION},
        {NULL, 0, NULL, 0}
    };

    int c;
    while((c = getopt_long(argc, argv, "m:pvZ:", longopts, NULL)) != -1) {
        switch(c) {
        case 'm':
            if(parse_mode(optarg, &opt_mode) != 0) {
                fprintf(stderr, "mkdir: invalid mode '%s'\n", optarg);
                usage(1);
            }
            opt_mode_given = 1;
            break;
        case 'p': opt_parents = 1; break;
        case 'v': opt_verbose = 1; break;
        case 'Z': break; /* SELinux/SMACK context: accepted and ignored */
        case OPT_HELP: usage(0); break;
        case OPT_VERSION:
            fputs("mkdir (EwokOS) 1.0\n", stdout);
            return 0;
        case '?':
            if(optopt != 0)
                fprintf(stderr, "mkdir: invalid option -- '%c'\n", (char)optopt);
            usage(1);
            break;
        default:
            usage(1);
        }
    }

    if(optind >= argc) {
        fprintf(stderr, "mkdir: missing operand\n");
        usage(1);
    }

    mode_t u = umask(0);
    umask(u);
    mode_t def_mode   = (mode_t)(0777 & ~u);          /* default final mode */
    mode_t mid_mode   = def_mode | S_IWUSR | S_IXUSR;  /* parents: umask + u+wx */
    mode_t final_mode = opt_mode_given ? opt_mode : def_mode;

    int ret = 0;
    for(int i = optind; i < argc; i++) {
        char path[FS_FULL_NAME_MAX + 1];
        char disp[FS_FULL_NAME_MAX + 1];
        if(normalize_operand(argv[i], path, sizeof(path)) != 0) {
            report_cannot(argv[i], ENOENT);
            ret = 1;
            continue;
        }
        strcpy(disp, path);
        if(make_dir(path, disp, final_mode, mid_mode) != 0)
            ret = 1;
    }
    return ret;
}
