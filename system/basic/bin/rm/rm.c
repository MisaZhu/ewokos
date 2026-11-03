/* rm - remove files or directories.
   Command line, diagnostics and exit status compatible with GNU coreutils rm.

   Platform notes (EwokOS):
    - there is no fts(3): a recursive removal walks a vfs_kids() snapshot and
      rebuilds each child path in place, so the depth is bounded by the stack
      and by FS_FULL_NAME_MAX;
    - a "device" is the VFS mount owner (fsinfo_t.mount_pid); that is what
      --one-file-system and --preserve-root=all compare;
    - write protection is derived from the permission bits in fsinfo_t.stat and
      the caller uid/gid, the same rules vfsd_check_access() applies, so a
      driver hiccup can never turn into a bogus diagnostic;
    - the getopt_long() this binary links (libc/libgloss/compat.c) neither
      permutes argv nor accepts abbreviated long options, both of which GNU
      getopt does; prepare_argv() supplies them here instead of changing the
      libc every other tool is built against.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <ewoksys/vfs.h>

/* Per entry outcome, mirroring GNU's enum RM_status (src/remove.h). A declined
   prompt is not an error, but it does keep every ancestor directory in place,
   exactly like GNU's mark_ancestor_dirs(). */
#define RM_OK            0
#define RM_USER_DECLINED 1
#define RM_ERROR        (-1)

/* Prompting modes, mirroring GNU's RMI_* (src/remove.h). */
enum { RMI_NEVER = 0, RMI_SOMETIMES, RMI_ALWAYS };

/* Long options that have no short equivalent. */
enum {
    OPT_INTERACTIVE = 256,
    OPT_ONE_FILE_SYSTEM,
    OPT_NO_PRESERVE_ROOT,
    OPT_PRESERVE_ROOT,
    OPT_HELP,
    OPT_VERSION
};

/* Every path this program builds lives in a buffer of this size. */
#define PATH_BUF (FS_FULL_NAME_MAX + 1)

static struct option const g_long_opts[] = {
    {"force",            no_argument,       NULL, 'f'},
    {"interactive",      optional_argument, NULL, OPT_INTERACTIVE},
    {"one-file-system",  no_argument,       NULL, OPT_ONE_FILE_SYSTEM},
    {"no-preserve-root", no_argument,       NULL, OPT_NO_PRESERVE_ROOT},
    {"preserve-root",    optional_argument, NULL, OPT_PRESERVE_ROOT},
    {"recursive",        no_argument,       NULL, 'r'},
    {"dir",              no_argument,       NULL, 'd'},
    {"verbose",          no_argument,       NULL, 'v'},
    {"help",             no_argument,       NULL, OPT_HELP},
    {"version",          no_argument,       NULL, OPT_VERSION},
    {NULL, 0, NULL, 0}
};

static struct {
    int recursive;          /* -r, -R, --recursive */
    int remove_empty_dirs;  /* -d, --dir */
    int ignore_missing;     /* -f, --force */
    int interactive;        /* RMI_* */
    int prompt_once;        /* -I */
    int one_file_system;    /* --one-file-system */
    int preserve_root;      /* cleared by --no-preserve-root only */
    int preserve_all_root;  /* --preserve-root=all */
    int verbose;            /* -v */
    int stdin_tty;          /* isatty(STDIN_FILENO) */
} g_x;

/* Long option names rebuilt by expand_long_option(); freed before exit. */
static char** g_built;
static int g_nbuilt;
static int g_built_max;

static void usage(int status) {
    if(status != 0) {
        fprintf(stderr, "Try 'rm --help' for more information.\n");
        exit(status);
    }
    fputs("Usage: rm [OPTION]... [FILE]...\n"
          "Remove (unlink) the FILE(s).\n"
          "\n"
          "  -f, --force\n"
          "         ignore nonexistent files and arguments, never prompt\n"
          "  -i\n"
          "         prompt before every removal\n"
          "  -I\n"
          "         prompt once before removing more than three files,\n"
          "         or when removing recursively; less intrusive than -i,\n"
          "         while still giving protection against most mistakes\n"
          "      --interactive[=WHEN]\n"
          "         prompt according to WHEN: never, once (-I), or always (-i);\n"
          "         without WHEN, prompt always\n"
          "      --one-file-system\n"
          "         when removing a hierarchy recursively,\n"
          "         skip any directory that is on a file system different\n"
          "         from that of the corresponding command line argument\n"
          "      --no-preserve-root\n"
          "         do not treat '/' specially\n"
          "      --preserve-root[=all]\n"
          "         do not remove '/' (default);\n"
          "         with 'all', reject any command line argument\n"
          "         on a separate device from its parent\n"
          "  -r, -R, --recursive\n"
          "         remove directories and their contents recursively\n"
          "  -d, --dir\n"
          "         remove empty directories\n"
          "  -v, --verbose\n"
          "         explain what is being done\n"
          "      --help     display this help and exit\n"
          "      --version  output version information and exit\n"
          "\n"
          "By default, rm does not remove directories.  Use the --recursive (-r or -R)\n"
          "option to remove each listed directory, too, along with all of its contents.\n"
          "\n"
          "Any attempt to remove a file whose last file name component is '.' or '..'\n"
          "is rejected with a diagnostic.\n"
          "\n"
          "To remove a file whose name starts with a '-', for example '-foo',\n"
          "use one of these commands:\n"
          "  rm -- -foo\n"
          "\n"
          "  rm ./-foo\n", stdout);
    exit(0);
}

/* The type name GNU rm puts into its prompts (gnulib lib/file-type.h). */
static const char* file_type_text(uint32_t type) {
    switch(FS_BASE_TYPE(type)) {
    case FS_TYPE_DIR:   return "directory";
    case FS_TYPE_FILE:  return "regular file";
    case FS_TYPE_LINK:  return "symbolic link";
    case FS_TYPE_PIPE:  return "named pipe (FIFO)";
    case FS_TYPE_CHAR:  return "character special file";
    case FS_TYPE_BLOCK: return "block special file";
    default:            return "weird file";
    }
}

/* strerror() on EwokOS is lower case and short on wording; GNU rm prints the
   Linux strings, which scripts and users recognize. */
static const char* err_text(int err) {
    switch(err) {
    case ENOENT:       return "No such file or directory";
    case EISDIR:       return "Is a directory";
    case ENOTDIR:      return "Not a directory";
    case ENOTEMPTY:    return "Directory not empty";
    case EACCES:
    case EPERM:        return "Permission denied";
    case EBUSY:        return "Device or resource busy";
    case EROFS:        return "Read-only file system";
    case ENAMETOOLONG: return "File name too long";
    case ENOSPC:       return "No space left on device";
    case EIO:          return "Input/output error";
    case ENOMEM:       return "Cannot allocate memory";
    case EXDEV:        return "Invalid cross-device link";
    case EINVAL:       return "Invalid argument";
    default:           return strerror(err);
    }
}

static void rm_error(const char* path, int err) {
    fprintf(stderr, "rm: cannot remove '%s': %s\n", path, err_text(err));
}

/* GNU's nonexistent_file_errno(): with -f these mean "it is not there". */
static int nonexistent_errno(int err) {
    return err == ENOENT || err == ENOTDIR || err == EINVAL || err == EILSEQ;
}

static const char* last_component(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash == NULL ? path : slash + 1;
}

static int dot_or_dotdot(const char* name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* True when PATH resolves to the root of the namespace: "/", "//", or "./"
   while the working directory is "/". GNU compares the (dev,ino) pair against
   "/"; the resolved name is what EwokOS provides cheaply. */
static int path_is_root(const char* path) {
    char abs[PATH_BUF];
    vfs_fullname(path, abs, FS_FULL_NAME_MAX);
    abs[FS_FULL_NAME_MAX] = 0;
    if(abs[0] != '/')
        return 0;
    for(const char* p = abs; *p != 0; p++) {
        if(*p != '/')
            return 0;
    }
    return 1;
}

/* Read one answer from stdin. Only a leading y/Y means yes; EOF, an empty line
   and anything else mean no, so a closed or piped stdin can never delete. */
static int yesno(void) {
    fflush(stdout);
    int c = getchar();
    int answer = (c == 'y' || c == 'Y');
    while(c != '\n' && c != EOF)
        c = getchar();
    return answer;
}

static int ask_remove(const char* type_text, const char* path, int wp) {
    fprintf(stderr, wp ? "rm: remove write-protected %s '%s'? "
                       : "rm: remove %s '%s'? ", type_text, path);
    fflush(stderr);
    return yesno();
}

static int ask_descend(const char* path, int wp) {
    fprintf(stderr, wp ? "rm: descend into write-protected directory '%s'? "
                       : "rm: descend into directory '%s'? ", path);
    fflush(stderr);
    return yesno();
}

/* True when the caller may not write INFO, mirroring vfsd_check_access(): uid 0
   writes anything, otherwise the owner/group/other bit decides. */
static int write_protected(const fsinfo_t* info) {
    if(FS_IS_TYPE(info->type, FS_TYPE_LINK))
        return 0;   /* permissions mean nothing for a link, as in GNU */
    uid_t uid = getuid();
    if(uid == 0)
        return 0;   /* can_write_any_file() */
    if(uid == info->stat.uid)
        return (info->stat.mode & S_IWUSR) ? 0 : 1;
    if(getgid() == info->stat.gid)
        return (info->stat.mode & S_IWGRP) ? 0 : 1;
    return (info->stat.mode & S_IWOTH) ? 0 : 1;
}

/* GNU prompts for a write protected entry even without -i, but only when the
   answer could come from a terminal. *WP receives that flag for the message. */
static int needs_prompt(const fsinfo_t* info, int* wp) {
    *wp = 0;
    if(!g_x.ignore_missing && (g_x.interactive == RMI_ALWAYS || g_x.stdin_tty))
        *wp = write_protected(info);
    return *wp != 0 || g_x.interactive == RMI_ALWAYS;
}

/* Remove the directory PATH, which is known to hold nothing left to remove. */
static int rm_dir(char* path, const fsinfo_t* info) {
    int wp;
    if(needs_prompt(info, &wp) && !ask_remove("directory", path, wp))
        return RM_USER_DECLINED;

    errno = 0;
    if(rmdir(path) != 0) {
        int err = errno != 0 ? errno : EPERM;
        if(g_x.ignore_missing && nonexistent_errno(err))
            return RM_OK;
        rm_error(path, err);
        return RM_ERROR;
    }
    if(g_x.verbose)
        printf("removed directory '%s'\n", path);
    return RM_OK;
}

/* -d without -r: an empty directory goes, a non-empty one is refused the way
   GNU refuses it, with EISDIR and before any prompt. */
static int rm_empty_dir(char* path, const fsinfo_t* info) {
    fsinfo_t self = *info;
    uint32_t num = (uint32_t)-1;
    fsinfo_t* kids = vfs_kids(&self, &num);
    if(kids == NULL && num != 0) {
        rm_error(path, EIO);
        return RM_ERROR;
    }
    int entries = 0;
    for(uint32_t i = 0; i < num; i++) {
        if(kids[i].name[0] != 0 && !dot_or_dotdot(kids[i].name))
            entries++;
    }
    free(kids);

    if(entries != 0) {
        rm_error(path, EISDIR);
        return RM_ERROR;
    }
    return rm_dir(path, info);
}

/*
 * Remove PATH, whose VFS record is *INFO, recursively. DEV is the mount owner
 * of the command line argument this tree belongs to, the "device" that
 * --one-file-system compares against. PATH is rewritten in place while the
 * children are walked and restored before returning, which keeps the stack
 * frame small no matter how deep the tree is.
 */
static int rm_tree(char* path, const fsinfo_t* info, int dev);

static int rm_node(char* path, const fsinfo_t* info, int dev) {
    if(FS_IS_TYPE(info->type, FS_TYPE_DIR)) {
        if(!g_x.recursive) {
            if(!g_x.remove_empty_dirs) {
                rm_error(path, EISDIR);
                return RM_ERROR;
            }
            return rm_empty_dir(path, info);
        }
        if(g_x.one_file_system && info->mount_pid != dev) {
            /* A mount point below the argument: stop here, as GNU does. */
            fprintf(stderr, "rm: skipping '%s', since it's on a different device\n", path);
            return RM_ERROR;
        }
        return rm_tree(path, info, dev);
    }

    /* Everything that is not a directory goes through unlink(). */
    int wp;
    if(needs_prompt(info, &wp) &&
            !ask_remove(file_type_text(info->type), path, wp))
        return RM_USER_DECLINED;

    errno = 0;
    if(unlink(path) != 0) {
        int err = errno != 0 ? errno : EPERM;
        if(g_x.ignore_missing && nonexistent_errno(err))
            return RM_OK;
        rm_error(path, err);
        return RM_ERROR;
    }
    if(g_x.verbose)
        printf("removed '%s'\n", path);
    return RM_OK;
}

static int rm_tree(char* path, const fsinfo_t* info, int dev) {
    /* Snapshot the kids: removing an entry changes the live directory list. */
    fsinfo_t self = *info;
    uint32_t num = (uint32_t)-1;
    fsinfo_t* kids = vfs_kids(&self, &num);
    if(kids == NULL && num != 0) {
        /* The list is unavailable, so the directory cannot be emptied. */
        rm_error(path, EIO);
        return RM_ERROR;
    }

    int entries = 0;
    for(uint32_t i = 0; i < num; i++) {
        if(kids[i].name[0] != 0 && !dot_or_dotdot(kids[i].name))
            entries++;
    }

    /* GNU offers an empty directory for removal straight away and never asks
       whether to descend into it. */
    if(entries == 0) {
        free(kids);
        return rm_dir(path, info);
    }

    int wp;
    if(needs_prompt(info, &wp) && !ask_descend(path, wp)) {
        free(kids);
        return RM_USER_DECLINED;
    }

    int status = RM_OK;
    size_t len = strlen(path);
    for(uint32_t i = 0; i < num; i++) {
        const char* name = kids[i].name;
        if(name[0] == 0 || dot_or_dotdot(name))
            continue;
        if(strchr(name, '/') != NULL ||
                len + 1 + strlen(name) >= (size_t)PATH_BUF) {
            /* A malformed VFS entry: never build a path out of it. */
            fprintf(stderr, "rm: cannot remove '%s/%s': %s\n", path, name,
                    err_text(ENAMETOOLONG));
            status = RM_ERROR;
            continue;
        }

        path[len] = '/';
        strcpy(path + len + 1, name);
        int s = rm_node(path, &kids[i], dev);
        path[len] = 0;

        if(s == RM_ERROR)
            status = RM_ERROR;
        else if(s == RM_USER_DECLINED && status != RM_ERROR)
            status = RM_USER_DECLINED;
    }
    free(kids);

    /* Something is left inside: keep the directory and stay quiet about it, the
       child diagnostic already said why (GNU's mark_ancestor_dirs). */
    if(status != RM_OK)
        return status;

    return rm_dir(path, info);
}

/* --preserve-root=all: refuse an argument that is a mount point, i.e. one whose
   device differs from its parent's. Returns 1 when the argument was skipped,
   with the diagnostic already printed. */
static int preserve_all_root_skip(const char* path, const fsinfo_t* info) {
    char parent[PATH_BUF];
    int n = snprintf(parent, sizeof(parent), "%s/..", path);
    if(n < 0 || (size_t)n >= sizeof(parent)) {
        fprintf(stderr, "rm: failed to stat '%s/..': skipping '%s'\n", path, path);
        return 1;
    }

    fsinfo_t pinfo;
    if(vfs_get_by_name(parent, &pinfo) != 0) {
        fprintf(stderr, "rm: failed to stat '%s': skipping '%s'\n", parent, path);
        return 1;
    }
    if(pinfo.mount_pid == info->mount_pid)
        return 0;

    fprintf(stderr, "rm: skipping '%s', since it's on a different device\n", path);
    fprintf(stderr, "rm: and --preserve-root=all is in effect\n");
    return 1;
}

/* Remove one command line argument. PATH is rewritten in place while a tree is
   walked, and holds the argument again on return. */
static int rm_argument(char* path) {
    fsinfo_t info;
    if(vfs_get_by_name(path, &info) != 0) {
        if(g_x.ignore_missing)
            return RM_OK;
        rm_error(path, ENOENT);
        return RM_ERROR;
    }

    /* The checks below apply to command line arguments only, and - like GNU's -
       only to directories that are about to be traversed. */
    if(FS_IS_TYPE(info.type, FS_TYPE_DIR) && g_x.recursive) {
        if(dot_or_dotdot(last_component(path))) {
            fprintf(stderr,
                    "rm: refusing to remove '.' or '..' directory: skipping '%s'\n",
                    path);
            return RM_ERROR;
        }
        if(g_x.preserve_root && path_is_root(path)) {
            fprintf(stderr, "rm: it is dangerous to operate recursively on '%s'\n", path);
            fprintf(stderr, "rm: use --no-preserve-root to override safety checks\n");
            return RM_ERROR;
        }
        if(g_x.preserve_all_root && preserve_all_root_skip(path, &info))
            return RM_ERROR;
    }

    return rm_node(path, &info, info.mount_pid);
}

/* --interactive=WHEN, matched like GNU's XARGMATCH table: an unambiguous
   abbreviation of a listed value is accepted. Returns an RMI_* mode, or -1. */
static int match_when(const char* arg) {
    static const char* const args[] = {
        "never", "no", "none", "once", "always", "yes", NULL
    };
    static const int vals[] = {
        RMI_NEVER, RMI_NEVER, RMI_NEVER, RMI_SOMETIMES, RMI_ALWAYS, RMI_ALWAYS
    };

    size_t len = strlen(arg);
    int found = -1;
    for(int i = 0; args[i] != NULL; i++) {
        if(strncmp(args[i], arg, len) != 0)
            continue;
        if(args[i][len] == 0)
            return vals[i];   /* an exact match beats every abbreviation */
        if(found >= 0)
            return -1;        /* ambiguous */
        found = i;
    }
    return found < 0 ? -1 : vals[found];
}

/* GNU forbids abbreviating this one option: it is the difference between a
   mistake and a dead system. */
static void reject_abbreviated_no_preserve_root(void) {
    fprintf(stderr, "rm: you may not abbreviate the --no-preserve-root option\n");
    exit(1);
}

/* Rewrite an abbreviated long option ("--rec") into the full name GNU getopt
   would have matched. Unambiguous prefixes only; an unknown or ambiguous name
   is left alone so that getopt_long() reports it. */
static char* expand_long_option(const char* arg) {
    if(arg[0] != '-' || arg[1] != '-' || arg[2] == 0)
        return (char*)arg;

    const char* eq = strchr(arg, '=');
    size_t name_len = eq == NULL ? strlen(arg) : (size_t)(eq - arg);
    const char* name = arg + 2;
    name_len -= 2;

    const struct option* found = NULL;
    for(int i = 0; g_long_opts[i].name != NULL; i++) {
        const char* opt = g_long_opts[i].name;
        if(strlen(opt) == name_len && strncmp(opt, name, name_len) == 0)
            return (char*)arg;   /* already the full name */
        if(strncmp(opt, name, name_len) == 0) {
            if(found != NULL)
                return (char*)arg;   /* ambiguous: let getopt_long() fail */
            found = &g_long_opts[i];
        }
    }
    if(found == NULL)
        return (char*)arg;

    if(strcmp(found->name, "no-preserve-root") == 0)
        reject_abbreviated_no_preserve_root();

    size_t need = strlen(found->name) + strlen(arg + name_len + 2) + 3;
    char* built = (char*)malloc(need);
    if(built == NULL)
        return (char*)arg;
    snprintf(built, need, "--%s%s", found->name, eq == NULL ? "" : eq);
    if(g_nbuilt >= g_built_max) {   /* cannot happen: one slot per argument */
        free(built);
        return (char*)arg;
    }
    g_built[g_nbuilt++] = built;
    return built;
}

static int is_option_arg(const char* arg) {
    /* A lone "-" names a file called "-", exactly as in GNU. */
    return arg[0] == '-' && arg[1] != 0;
}

/*
 * Reorder argv the way GNU getopt would have: every option first, then every
 * operand, with "--" dropped and everything past it forced to be an operand.
 * Long options are expanded to their full name on the way. Returns the new
 * argument count; argv itself is rewritten in place.
 */
static int prepare_argv(int argc, char** argv) {
    if(argc <= 1)
        return argc;

    char** head = (char**)malloc(sizeof(char*) * (size_t)argc);
    char** tail = (char**)malloc(sizeof(char*) * (size_t)argc);
    g_built = (char**)malloc(sizeof(char*) * (size_t)argc);
    if(head == NULL || tail == NULL || g_built == NULL) {
        free(head);
        free(tail);
        free(g_built);
        g_built = NULL;
        g_built_max = 0;
        return argc;   /* out of memory: options must then precede operands */
    }
    g_nbuilt = 0;
    g_built_max = argc;

    int nh = 0, nt = 0;
    for(int pass = 0; pass < 2; pass++) {
        int past_separator = 0;
        for(int i = 1; i < argc; i++) {
            char* arg = argv[i];
            if(past_separator) {
                if(pass != 0)
                    tail[nt++] = arg;
                continue;
            }
            if(strcmp(arg, "--") == 0) {
                past_separator = 1;
                continue;
            }
            if(is_option_arg(arg)) {
                if(pass == 0)
                    head[nh++] = expand_long_option(arg);
            }
            else if(pass != 0) {
                tail[nt++] = arg;
            }
        }
    }

    for(int i = 0; i < nh; i++)
        argv[1 + i] = head[i];
    for(int i = 0; i < nt; i++)
        argv[1 + nh + i] = tail[i];
    free(head);
    free(tail);
    return 1 + nh + nt;
}

static void free_built_options(void) {
    for(int i = 0; i < g_nbuilt; i++)
        free(g_built[i]);
    g_nbuilt = 0;
    g_built_max = 0;
    free(g_built);
    g_built = NULL;
}

/* GNU advises about "rm -foo" when a file called "-foo" really exists. */
static void diagnose_leading_hyphen(int argc, char** argv) {
    for(int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        fsinfo_t info;
        if(arg[0] == '-' && arg[1] != 0 && vfs_get_by_name(arg, &info) == 0) {
            fprintf(stderr, "Try 'rm ./%s' to remove the file '%s'.\n", arg, arg);
            return;
        }
    }
}

int main(int argc, char** argv) {
    int c;

    g_x.interactive = RMI_SOMETIMES;
    g_x.preserve_root = 1;
    g_x.stdin_tty = isatty(STDIN_FILENO);

    argc = prepare_argv(argc, argv);

    while((c = getopt_long(argc, argv, "dfirvIR", g_long_opts, NULL)) != -1) {
        switch(c) {
        case 'd':
            g_x.remove_empty_dirs = 1;
            break;
        case 'f':
            g_x.interactive = RMI_NEVER;
            g_x.ignore_missing = 1;
            g_x.prompt_once = 0;
            break;
        case 'i':
            g_x.interactive = RMI_ALWAYS;
            g_x.ignore_missing = 0;
            g_x.prompt_once = 0;
            break;
        case 'I':
            g_x.interactive = RMI_SOMETIMES;
            g_x.ignore_missing = 0;
            g_x.prompt_once = 1;
            break;
        case 'r':
        case 'R':
            g_x.recursive = 1;
            break;
        case 'v':
            g_x.verbose = 1;
            break;
        case OPT_INTERACTIVE:
            if(optarg == NULL) {
                g_x.interactive = RMI_ALWAYS;
                g_x.ignore_missing = 0;
                g_x.prompt_once = 0;
                break;
            }
            {
                int mode = match_when(optarg);
                if(mode < 0) {
                    fprintf(stderr, "rm: invalid argument '%s' for '--interactive'\n",
                            optarg);
                    fprintf(stderr,
                            "Valid arguments are: 'never', 'once', 'always'\n");
                    usage(1);
                }
                g_x.interactive = mode;
                g_x.ignore_missing = 0;
                g_x.prompt_once = (mode == RMI_SOMETIMES);
            }
            break;
        case OPT_ONE_FILE_SYSTEM:
            g_x.one_file_system = 1;
            break;
        case OPT_NO_PRESERVE_ROOT:
            g_x.preserve_root = 0;
            break;
        case OPT_PRESERVE_ROOT:
            if(optarg != NULL) {
                if(strcmp(optarg, "all") == 0)
                    g_x.preserve_all_root = 1;
                else {
                    fprintf(stderr,
                            "rm: unrecognized --preserve-root argument: '%s'\n",
                            optarg);
                    usage(1);
                }
            }
            g_x.preserve_root = 1;
            break;
        case OPT_HELP:
            usage(0);
            break;
        case OPT_VERSION:
            fputs("rm (EwokOS) 1.0\n", stdout);
            free_built_options();
            return 0;
        default:
            {
                const char* bad = (optind > 0 && optind <= argc) ?
                        argv[optind - 1] : "";
                if(bad[0] == '-' && bad[1] == '-')
                    fprintf(stderr, "rm: unrecognized option '%s'\n", bad);
                else if(optopt != 0)
                    fprintf(stderr, "rm: invalid option -- '%c'\n", (char)optopt);
                else
                    fprintf(stderr, "rm: unrecognized option '%s'\n", bad);
                diagnose_leading_hyphen(argc, argv);
                usage(1);
            }
        }
    }

    int nfiles = argc - optind;
    if(nfiles <= 0) {
        if(g_x.ignore_missing) {
            free_built_options();
            return 0;   /* "rm -f" on its own is a success, as in GNU */
        }
        fprintf(stderr, "rm: missing operand\n");
        usage(1);
    }

    if(g_x.prompt_once && (g_x.recursive || nfiles > 3)) {
        fprintf(stderr, "rm: remove %d argument%s%s? ", nfiles,
                nfiles == 1 ? "" : "s", g_x.recursive ? " recursively" : "");
        fflush(stderr);
        if(!yesno()) {
            free_built_options();
            return 0;
        }
    }

    int status = RM_OK;
    char path[PATH_BUF];
    for(int i = optind; i < argc; i++) {
        const char* arg = argv[i];
        size_t len = strlen(arg);

        /* Drop trailing slashes so that "dir//" and "dir/" behave like "dir",
           but keep a name made only of slashes: that is the root. */
        while(len > 1 && arg[len - 1] == '/')
            len--;
        if(len >= sizeof(path)) {
            rm_error(arg, ENAMETOOLONG);
            status = RM_ERROR;
            continue;
        }
        memcpy(path, arg, len);
        path[len] = 0;

        int s = rm_argument(path);
        if(s == RM_ERROR)
            status = RM_ERROR;
        else if(s == RM_USER_DECLINED && status != RM_ERROR)
            status = RM_USER_DECLINED;
    }

    free_built_options();
    return status == RM_ERROR ? 1 : 0;
}
