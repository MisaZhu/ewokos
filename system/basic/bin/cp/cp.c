/*
 * cp - copy files and directories, GNU/Linux command-line compatible.
 *
 * Supports the three coreutils forms:
 *   cp [OPTION]... [-T] SOURCE DEST
 *   cp [OPTION]... SOURCE... DIRECTORY
 *   cp [OPTION]... -t DIRECTORY SOURCE...
 *
 * EwokOS-specific notes:
 *  - The VFS has no symbolic or hard links: symlink()/link()/readlink() fail
 *    with ENOSYS. -s/-l are therefore accepted for CLI compatibility but report
 *    the failure the way GNU cp reports a failed link. lstat()==stat(), so the
 *    -H/-L/-P/-d dereference selectors parse and behave, but change nothing.
 *  - utimes() is a no-op stub, so -p/--preserve=timestamps is accepted and
 *    called but the VFS cannot store the result.
 *  - st_dev is always 0, so -x (one file system) never skips anything.
 *  - _open() applies the mode argument only when it creates a file, and filters
 *    it through the umask; the exact source mode is restored with fchmod() for
 *    new files, while an existing destination keeps its own mode unless -p.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ewoksys/vfs.h>

#define PROG "cp"
#define CP_BUF_SIZE (1024 * 256)

/* Long-only option codes, kept out of the ASCII range so they never collide
 * with a short-option character returned by getopt_long(). */
enum {
    OPT_BACKUP = 256,
    OPT_COPY_CONTENTS,
    OPT_NO_PRESERVE,
    OPT_PRESERVE,
    OPT_PARENTS,
    OPT_REFLINK,
    OPT_REMOVE_DEST,
    OPT_SPARSE,
    OPT_STRIP_SLASHES,
    OPT_HELP,
    OPT_VERSION
};

/* -i / -n are mutually exclusive; the last one given wins, as in GNU cp. */
enum {
    OVERWRITE_DEFAULT = 0,
    OVERWRITE_PROMPT,     /* -i */
    OVERWRITE_NEVER       /* -n */
};

/* ── option state ── */
static int opt_recursive = 0;         /* -r / -R / --recursive */
static int opt_force = 0;             /* -f */
static int opt_update = 0;            /* -u */
static int opt_verbose = 0;           /* -v */
static int opt_hard_link = 0;         /* -l */
static int opt_sym_link = 0;          /* -s */
static int opt_dereference = 0;       /* -H / -L */
static int opt_no_target_dir = 0;     /* -T */
static int opt_one_file_system = 0;   /* -x */
static int opt_copy_contents = 0;     /* --copy-contents */
static int opt_parents = 0;           /* --parents */
static int opt_remove_dest = 0;       /* --remove-destination */
static int opt_backup = 0;            /* -b / --backup */
static int overwrite_mode = OVERWRITE_DEFAULT;
static const char* opt_backup_suffix = "~";  /* -S */
static const char* opt_target_dir = NULL;    /* -t DIR */

/* --preserve attribute selectors */
static int pres_mode = 0;
static int pres_owner = 0;
static int pres_time = 0;
static int pres_links = 0;

/* runtime */
static int g_status = 0;   /* exit status; set to 1 once any copy fails */
static char* g_buf = NULL; /* shared data-copy buffer */

static const struct option long_opts[] = {
    {"archive",                no_argument,       0, 'a'},
    {"backup",                 optional_argument, 0, OPT_BACKUP},
    {"copy-contents",          no_argument,       0, OPT_COPY_CONTENTS},
    {"dereference",            no_argument,       0, 'L'},
    {"force",                  no_argument,       0, 'f'},
    {"help",                   no_argument,       0, OPT_HELP},
    {"interactive",            no_argument,       0, 'i'},
    {"link",                   no_argument,       0, 'l'},
    {"no-clobber",             no_argument,       0, 'n'},
    {"no-dereference",         no_argument,       0, 'P'},
    {"no-preserve",            required_argument, 0, OPT_NO_PRESERVE},
    {"no-target-directory",    no_argument,       0, 'T'},
    {"one-file-system",        no_argument,       0, 'x'},
    {"parents",                no_argument,       0, OPT_PARENTS},
    {"preserve",               optional_argument, 0, OPT_PRESERVE},
    {"recursive",              no_argument,       0, 'R'},
    {"reflink",                optional_argument, 0, OPT_REFLINK},
    {"remove-destination",     no_argument,       0, OPT_REMOVE_DEST},
    {"sparse",                 required_argument, 0, OPT_SPARSE},
    {"strip-trailing-slashes", no_argument,       0, OPT_STRIP_SLASHES},
    {"suffix",                 required_argument, 0, 'S'},
    {"symbolic-link",          no_argument,       0, 's'},
    {"target-directory",       required_argument, 0, 't'},
    {"update",                 no_argument,       0, 'u'},
    {"verbose",                no_argument,       0, 'v'},
    {"version",                no_argument,       0, OPT_VERSION},
    {0, 0, 0, 0}
};

/* ── small path helpers ── */

static void strip_trailing_slashes(char* path) {
    size_t len = strlen(path);
    while(len > 1 && path[len - 1] == '/')
        path[--len] = '\0';
}

/* Last path component, ignoring trailing slashes (basename(1) semantics). */
static const char* base_name(const char* path) {
    size_t len = strlen(path);
    const char* p;
    while(len > 1 && path[len - 1] == '/')
        len--;
    p = path + len;
    while(p > path && *(p - 1) != '/')
        p--;
    return p;
}

static int join_path(char* out, size_t outsz, const char* dir, const char* name) {
    int n;
    size_t dlen = strlen(dir);
    if(dlen > 0 && dir[dlen - 1] == '/')
        n = snprintf(out, outsz, "%s%s", dir, name);
    else
        n = snprintf(out, outsz, "%s/%s", dir, name);
    return (n < 0 || (size_t)n >= outsz) ? -1 : 0;
}

/* Resolve a command-line operand to an absolute path with trailing slashes
 * removed. vfs_fullname() is pure string work, so it is valid for a
 * destination that does not exist yet. */
static int resolve_abs(const char* arg, char* out, size_t outsz) {
    char tmp[FS_FULL_NAME_MAX + 1];
    memset(tmp, 0, sizeof(tmp));
    vfs_fullname(arg, tmp, FS_FULL_NAME_MAX);
    strip_trailing_slashes(tmp);
    if(tmp[0] == '\0' || strlen(tmp) >= outsz)
        return -1;
    strcpy(out, tmp);
    return 0;
}

static void report_copy(const char* src, const char* dst) {
    if(opt_verbose)
        printf("'%s' -> '%s'\n", src, dst);
}

static int prompt_yes(const char* dst) {
    char line[16];
    fprintf(stderr, "%s: overwrite '%s'? ", PROG, dst);
    fflush(stderr);
    if(fgets(line, sizeof(line), stdin) == NULL)
        return 0;
    return (line[0] == 'y' || line[0] == 'Y');
}

static int backup_name(const char* dst, char* out, size_t outsz) {
    int n = snprintf(out, outsz, "%s%s", dst, opt_backup_suffix);
    return (n < 0 || (size_t)n >= outsz) ? -1 : 0;
}

/* ── --preserve parsing ── */

static int tok_is(const char* tok, size_t len, const char* word) {
    return strlen(word) == len && strncmp(tok, word, len) == 0;
}

static void preserve_set(const char* tok, size_t len, int on) {
    if(tok_is(tok, len, "all")) {
        pres_mode = pres_owner = pres_time = pres_links = on;
    } else if(tok_is(tok, len, "mode") || tok_is(tok, len, "permissions")) {
        pres_mode = on;
    } else if(tok_is(tok, len, "ownership")) {
        pres_owner = on;
    } else if(tok_is(tok, len, "timestamps")) {
        pres_time = on;
    } else if(tok_is(tok, len, "links")) {
        pres_links = on;
    }
    /* context / xattr are accepted and ignored: EwokOS stores neither. */
}

static void preserve_parse(const char* list, int on) {
    const char* p;
    if(list == NULL) {   /* -p, or --preserve with no argument */
        pres_mode = pres_owner = pres_time = on;
        return;
    }
    p = list;
    while(*p != '\0') {
        const char* comma = strchr(p, ',');
        size_t len = comma ? (size_t)(comma - p) : strlen(p);
        if(len > 0)
            preserve_set(p, len, on);
        if(comma == NULL)
            break;
        p = comma + 1;
    }
}

/* ── copy primitives ── */

static void preserve_apply(const char* dst, const struct stat* sst, int existed,
                           mode_t create_mode) {
    if(!existed || pres_mode)
        chmod(dst, create_mode);
    if(pres_owner)
        chown(dst, (int)sst->st_uid, (int)sst->st_gid);
    if(pres_time) {
        struct timeval tv[2];
        tv[0].tv_sec = sst->st_atime; tv[0].tv_usec = 0;
        tv[1].tv_sec = sst->st_mtime; tv[1].tv_usec = 0;
        utimes(dst, tv);
    }
}

static int copy_regular(const char* src, const char* dst, const struct stat* sst) {
    struct stat dst_st;
    int existed = (stat(dst, &dst_st) == 0);
    mode_t create_mode = pres_mode ? (sst->st_mode & 07777) : (sst->st_mode & 0777);
    int fd_in, fd_out, rc = 0;

    if(existed && overwrite_mode == OVERWRITE_NEVER)
        return 0;
    if(existed && opt_update && dst_st.st_mtime >= sst->st_mtime)
        return 0;
    if(existed && overwrite_mode == OVERWRITE_PROMPT && !prompt_yes(dst))
        return 0;

    if(existed && (opt_remove_dest || opt_backup)) {
        char bak[FS_FULL_NAME_MAX + 1];
        if(opt_backup) {
            if(backup_name(dst, bak, sizeof(bak)) != 0) {
                fprintf(stderr, "%s: backup name too long for '%s'\n", PROG, dst);
                g_status = 1;
                return -1;
            }
            if(rename(dst, bak) != 0) {
                fprintf(stderr, "%s: cannot backup '%s': %s\n", PROG, dst, strerror(errno));
                g_status = 1;
                return -1;
            }
        } else if(unlink(dst) != 0) {
            fprintf(stderr, "%s: cannot remove '%s': %s\n", PROG, dst, strerror(errno));
            g_status = 1;
            return -1;
        }
        existed = 0;
    }

    fd_in = open(src, O_RDONLY);
    if(fd_in < 0) {
        fprintf(stderr, "%s: cannot open '%s' for reading: %s\n", PROG, src, strerror(errno));
        g_status = 1;
        return -1;
    }

    fd_out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, create_mode);
    if(fd_out < 0 && opt_force) {
        unlink(dst);
        fd_out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, create_mode);
        existed = 0;
    }
    if(fd_out < 0) {
        fprintf(stderr, "%s: cannot create regular file '%s': %s\n", PROG, dst, strerror(errno));
        close(fd_in);
        g_status = 1;
        return -1;
    }

    report_copy(src, dst);

    while(1) {
        ssize_t rd = read(fd_in, g_buf, CP_BUF_SIZE);
        ssize_t off = 0;
        if(rd == 0)
            break;
        if(rd < 0) {
            fprintf(stderr, "%s: error reading '%s': %s\n", PROG, src, strerror(errno));
            rc = -1;
            break;
        }
        while(off < rd) {
            ssize_t wr = write(fd_out, g_buf + off, (size_t)(rd - off));
            if(wr > 0) { off += wr; continue; }
            if(wr < 0 && (errno == EAGAIN || errno == EINTR)) { usleep(1000); continue; }
            fprintf(stderr, "%s: error writing '%s': %s\n", PROG, dst, strerror(errno));
            rc = -1;
            goto out;
        }
    }

out:
    close(fd_in);
    /* A new file was created with (create_mode & ~umask); restore the exact
     * mode. An existing file keeps its own mode unless we are preserving. */
    if(rc == 0 && (!existed || pres_mode))
        fchmod(fd_out, create_mode);
    close(fd_out);

    if(rc == 0) {
        if(pres_owner)
            chown(dst, (int)sst->st_uid, (int)sst->st_gid);
        if(pres_time) {
            struct timeval tv[2];
            tv[0].tv_sec = sst->st_atime; tv[0].tv_usec = 0;
            tv[1].tv_sec = sst->st_mtime; tv[1].tv_usec = 0;
            utimes(dst, tv);
        }
    }

    if(rc != 0)
        g_status = 1;
    return rc;
}

static int copy_fifo(const char* src, const char* dst, const struct stat* sst) {
    struct stat dst_st;
    int existed = (stat(dst, &dst_st) == 0);
    mode_t m = pres_mode ? (sst->st_mode & 07777) : (sst->st_mode & 0777);

    if(existed && overwrite_mode == OVERWRITE_NEVER)
        return 0;
    if(existed && overwrite_mode == OVERWRITE_PROMPT && !prompt_yes(dst))
        return 0;
    if(existed && unlink(dst) != 0) {
        fprintf(stderr, "%s: cannot remove '%s': %s\n", PROG, dst, strerror(errno));
        g_status = 1;
        return -1;
    }
    if(mkfifo(dst, m) != 0) {
        fprintf(stderr, "%s: cannot create fifo '%s': %s\n", PROG, dst, strerror(errno));
        g_status = 1;
        return -1;
    }
    report_copy(src, dst);
    if(pres_owner)
        chown(dst, (int)sst->st_uid, (int)sst->st_gid);
    return 0;
}

static int make_symlink_to(const char* target, const char* dst) {
    if(symlink(target, dst) != 0) {
        fprintf(stderr, "%s: cannot create symbolic link '%s' -> '%s': %s\n",
                PROG, dst, target, strerror(errno));
        g_status = 1;
        return -1;
    }
    report_copy(target, dst);
    return 0;
}

static int make_hard_link(const char* src, const char* dst) {
    if(link(src, dst) != 0) {
        fprintf(stderr, "%s: cannot create link '%s' => '%s': %s\n",
                PROG, dst, src, strerror(errno));
        g_status = 1;
        return -1;
    }
    report_copy(src, dst);
    return 0;
}

static int copy_entry(const char* src, const char* dst, dev_t top_dev);

static int copy_directory(const char* src, const char* dst,
                          const struct stat* sst, dev_t top_dev) {
    struct stat dst_st;
    int dst_exists = (stat(dst, &dst_st) == 0);
    size_t slen = strlen(src);
    DIR* dp;
    struct dirent* de;
    int rc = 0;

    if(dst_exists && !S_ISDIR(dst_st.st_mode)) {
        fprintf(stderr, "%s: cannot overwrite non-directory '%s' with directory '%s'\n",
                PROG, dst, src);
        g_status = 1;
        return -1;
    }

    /* Refuse to copy a directory into itself (dst nested under src). */
    if(strncmp(dst, src, slen) == 0 && dst[slen] == '/') {
        fprintf(stderr, "%s: cannot copy a directory, '%s', into itself, '%s'\n",
                PROG, src, dst);
        g_status = 1;
        return -1;
    }

    if(!dst_exists) {
        /* u+wx so the tree can be populated even if the source is read-only;
         * the real mode is applied after the children are copied. */
        mode_t mk = (sst->st_mode & 07777) | S_IRWXU;
        if(mkdir(dst, mk) != 0) {
            fprintf(stderr, "%s: cannot create directory '%s': %s\n",
                    PROG, dst, strerror(errno));
            g_status = 1;
            return -1;
        }
    }

    report_copy(src, dst);

    dp = opendir(src);
    if(dp == NULL) {
        fprintf(stderr, "%s: cannot open directory '%s': %s\n", PROG, src, strerror(errno));
        g_status = 1;
        return -1;
    }
    while((de = readdir(dp)) != NULL) {
        char csrc[FS_FULL_NAME_MAX + 1];
        char cdst[FS_FULL_NAME_MAX + 1];
        const char* nm = de->d_name;
        if(nm[0] == '\0' || strcmp(nm, ".") == 0 || strcmp(nm, "..") == 0)
            continue;
        if(strchr(nm, '/') != NULL)
            continue;
        if(join_path(csrc, sizeof(csrc), src, nm) != 0 ||
           join_path(cdst, sizeof(cdst), dst, nm) != 0) {
            fprintf(stderr, "%s: path too long: '%s/%s'\n", PROG, src, nm);
            rc = -1;
            g_status = 1;
            continue;
        }
        if(copy_entry(csrc, cdst, top_dev) != 0)
            rc = -1;
    }
    closedir(dp);

    /* An existing destination directory keeps its mode unless preserving. */
    preserve_apply(dst, sst, dst_exists,
                   pres_mode ? (sst->st_mode & 07777) : (sst->st_mode & 0777));
    return rc;
}

static int copy_entry(const char* src, const char* dst, dev_t top_dev) {
    struct stat st;

    if(strcmp(src, dst) == 0) {
        fprintf(stderr, "%s: '%s' and '%s' are the same file\n", PROG, src, dst);
        g_status = 1;
        return -1;
    }

    /* EwokOS has no symlinks so stat()==lstat(); honour -H/-L vs -P/-d anyway. */
    {
        int sr = opt_dereference ? stat(src, &st) : lstat(src, &st);
        if(sr != 0) {
            fprintf(stderr, "%s: cannot stat '%s': %s\n", PROG, src, strerror(errno));
            g_status = 1;
            return -1;
        }
    }

    /* Symbolic links: dormant on EwokOS (lstat()==stat(), no link nodes) but
     * kept so -P/-d/--preserve=links are correct if the VFS ever grows them. */
    if(S_ISLNK(st.st_mode)) {
        if(!opt_dereference && (pres_links || opt_sym_link)) {
            char tgt[FS_FULL_NAME_MAX + 1];
            ssize_t n = readlink(src, tgt, sizeof(tgt) - 1);
            if(n < 0) {
                fprintf(stderr, "%s: cannot read symbolic link '%s': %s\n",
                        PROG, src, strerror(errno));
                g_status = 1;
                return -1;
            }
            tgt[n] = '\0';
            return make_symlink_to(tgt, dst);
        }
        if(stat(src, &st) != 0) {   /* dereference and copy the target */
            fprintf(stderr, "%s: cannot stat '%s': %s\n", PROG, src, strerror(errno));
            g_status = 1;
            return -1;
        }
    }

    if(S_ISDIR(st.st_mode)) {
        if(!opt_recursive) {
            fprintf(stderr, "%s: -r not specified; omitting directory '%s'\n", PROG, src);
            g_status = 1;
            return -1;
        }
        if(opt_one_file_system && st.st_dev != top_dev)
            return 0;
        return copy_directory(src, dst, &st, top_dev);
    }

    if(S_ISREG(st.st_mode)) {
        if(opt_sym_link)
            return make_symlink_to(src, dst);
        if(opt_hard_link)
            return make_hard_link(src, dst);
        return copy_regular(src, dst, &st);
    }

    if(S_ISFIFO(st.st_mode)) {
        if(opt_copy_contents)
            return copy_regular(src, dst, &st);
        return copy_fifo(src, dst, &st);
    }

    /* character / block / socket nodes: mknod() is unavailable on EwokOS. */
    fprintf(stderr, "%s: cannot copy special file '%s': Operation not supported\n",
            PROG, src);
    g_status = 1;
    return -1;
}

/* Create every directory in dst's parent chain that lies below base. */
static void mkdir_parents(const char* dst, const char* base) {
    char tmp[FS_FULL_NAME_MAX + 1];
    const char* last = strrchr(dst, '/');
    size_t blen = strlen(base);
    size_t plen, i;

    if(last == NULL)
        return;
    plen = (size_t)(last - dst);
    if(plen <= blen || plen >= sizeof(tmp))
        return;

    memcpy(tmp, dst, plen);
    tmp[plen] = '\0';
    for(i = blen + 1; i < plen; i++) {
        if(tmp[i] == '/') {
            tmp[i] = '\0';
            mkdir(tmp, 0777);   /* ignore EEXIST */
            tmp[i] = '/';
        }
    }
    mkdir(tmp, 0777);
}

/* Copy one source operand into an existing target directory. */
static void copy_into_dir(const char* src_arg, const char* tdir_abs) {
    char src_abs[FS_FULL_NAME_MAX + 1];
    char dst[FS_FULL_NAME_MAX + 1];
    struct stat sst;
    dev_t top_dev = 0;

    if(resolve_abs(src_arg, src_abs, sizeof(src_abs)) != 0) {
        fprintf(stderr, "%s: path too long: '%s'\n", PROG, src_arg);
        g_status = 1;
        return;
    }

    if(opt_parents) {
        /* --parents: recreate the source's leading path under the target. */
        const char* rel = src_arg;
        while(rel[0] == '.' && rel[1] == '/') { rel += 2; while(*rel == '/') rel++; }
        while(*rel == '/') rel++;
        if(rel[0] == '\0')
            rel = base_name(src_abs);
        if(join_path(dst, sizeof(dst), tdir_abs, rel) != 0) {
            fprintf(stderr, "%s: path too long: '%s'\n", PROG, src_arg);
            g_status = 1;
            return;
        }
        mkdir_parents(dst, tdir_abs);
    } else if(join_path(dst, sizeof(dst), tdir_abs, base_name(src_abs)) != 0) {
        fprintf(stderr, "%s: path too long: '%s'\n", PROG, src_arg);
        g_status = 1;
        return;
    }

    if(stat(src_abs, &sst) == 0)
        top_dev = sst.st_dev;
    copy_entry(src_abs, dst, top_dev);
}

/* Copy a single SOURCE onto DEST (DEST is a plain path, not a directory). */
static void copy_to_path(const char* src_arg, const char* dst_arg) {
    char src_abs[FS_FULL_NAME_MAX + 1];
    char dst_abs[FS_FULL_NAME_MAX + 1];
    struct stat sst;
    dev_t top_dev = 0;

    if(resolve_abs(src_arg, src_abs, sizeof(src_abs)) != 0 ||
       resolve_abs(dst_arg, dst_abs, sizeof(dst_abs)) != 0) {
        fprintf(stderr, "%s: path too long\n", PROG);
        g_status = 1;
        return;
    }
    if(stat(src_abs, &sst) == 0)
        top_dev = sst.st_dev;
    copy_entry(src_abs, dst_abs, top_dev);
}

/* ── help / version ── */

static void try_help(void) {
    fprintf(stderr, "Try '%s --help' for more information.\n", PROG);
}

static void help(void) {
    printf(
"Usage: %s [OPTION]... [-T] SOURCE DEST\n"
"  or:  %s [OPTION]... SOURCE... DIRECTORY\n"
"  or:  %s [OPTION]... -t DIRECTORY SOURCE...\n"
"Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n"
"\n"
"  -a, --archive                  same as -dR --preserve=all\n"
"      --backup[=CONTROL]         make a backup of each existing destination file\n"
"  -b                             like --backup but does not accept an argument\n"
"      --copy-contents            copy contents of special files when recursive\n"
"  -d                             same as --no-dereference --preserve=links\n"
"  -f, --force                    if destination cannot be opened, remove it and retry\n"
"  -i, --interactive              prompt before overwrite\n"
"  -H                             follow command-line symbolic links in SOURCE\n"
"  -l, --link                     hard link files instead of copying\n"
"  -L, --dereference              always follow symbolic links in SOURCE\n"
"  -n, --no-clobber               do not overwrite an existing file\n"
"  -P, --no-dereference           never follow symbolic links in SOURCE\n"
"  -p                             same as --preserve=mode,ownership,timestamps\n"
"      --preserve[=ATTR_LIST]     preserve the specified attributes\n"
"      --no-preserve=ATTR_LIST    don't preserve the specified attributes\n"
"      --parents                  use full source file name under DIRECTORY\n"
"  -r, -R, --recursive            copy directories recursively\n"
"      --reflink[=WHEN]           control clone/CoW copies (treated as plain copy)\n"
"      --remove-destination       remove each existing destination file first\n"
"      --sparse=WHEN              control creation of sparse files (plain copy)\n"
"      --strip-trailing-slashes   remove trailing slashes from each SOURCE\n"
"  -s, --symbolic-link            make symbolic links instead of copying\n"
"  -S, --suffix=SUFFIX            override the usual backup suffix\n"
"  -t, --target-directory=DIRECTORY  copy all SOURCE arguments into DIRECTORY\n"
"  -T, --no-target-directory      treat DEST as a normal file\n"
"  -u, --update                   copy only when SOURCE is newer or DEST is missing\n"
"  -v, --verbose                  explain what is being done\n"
"  -x, --one-file-system          stay on this file system\n"
"      --help                     display this help and exit\n"
"      --version                  output version information and exit\n"
"\n"
"ATTR_LIST may contain: mode, ownership, timestamps, links, all.\n"
"By default cp preserves nothing but the permission bits of a new file.\n"
"\n"
"Note: EwokOS has no symbolic or hard links, so -s/-l/-d report an error, and\n"
"timestamps cannot be stored, so -p/--preserve=timestamps has no effect.\n",
    PROG, PROG, PROG);
}

static void version(void) {
    printf("%s (EwokOS coreutils) 1.0\n", PROG);
}

/* ── main ── */

int main(int argc, char** argv) {
    int c;
    int nops;
    char** ops;
    char tdir[FS_FULL_NAME_MAX + 1];
    int into_dir = 0;
    char** srcs = NULL;
    int nsrcs = 0;
    char* src_arg = NULL;
    char* dst_arg = NULL;
    int i;

    while((c = getopt_long(argc, argv, "abdfHilLnprRsS:t:Tuvx", long_opts, NULL)) != -1) {
        switch(c) {
            case 'a':
                opt_recursive = 1;
                pres_mode = pres_owner = pres_time = pres_links = 1;
                break;
            case 'b': opt_backup = 1; break;
            case 'd': pres_links = 1; break;
            case 'f': opt_force = 1; break;
            case 'H':
            case 'L': opt_dereference = 1; break;
            case 'i': overwrite_mode = OVERWRITE_PROMPT; break;
            case 'l': opt_hard_link = 1; break;
            case 'n': overwrite_mode = OVERWRITE_NEVER; break;
            case 'p': pres_mode = pres_owner = pres_time = 1; break;
            case 'P': opt_dereference = 0; break;
            case 'r':
            case 'R': opt_recursive = 1; break;
            case 's': opt_sym_link = 1; break;
            case 'S': opt_backup_suffix = optarg; opt_backup = 1; break;
            case 't': opt_target_dir = optarg; break;
            case 'T': opt_no_target_dir = 1; break;
            case 'u': opt_update = 1; break;
            case 'v': opt_verbose = 1; break;
            case 'x': opt_one_file_system = 1; break;
            case OPT_BACKUP:
                if(optarg != NULL &&
                   (strcmp(optarg, "none") == 0 || strcmp(optarg, "off") == 0))
                    opt_backup = 0;
                else
                    opt_backup = 1;
                break;
            case OPT_COPY_CONTENTS: opt_copy_contents = 1; break;
            case OPT_PRESERVE:      preserve_parse(optarg, 1); break;
            case OPT_NO_PRESERVE:   preserve_parse(optarg, 0); break;
            case OPT_PARENTS:       opt_parents = 1; break;
            case OPT_REMOVE_DEST:   opt_remove_dest = 1; break;
            case OPT_REFLINK:       /* no CoW on EwokOS: plain copy */ break;
            case OPT_SPARSE:        /* plain copy */ break;
            case OPT_STRIP_SLASHES: /* operands are normalized in resolve_abs */ break;
            case OPT_HELP:          help(); return 0;
            case OPT_VERSION:       version(); return 0;
            case '?':
            default:
                if(optopt > 32 && optopt < 127)
                    fprintf(stderr, "%s: invalid option -- '%c'\n", PROG, optopt);
                else
                    fprintf(stderr, "%s: invalid option\n", PROG);
                try_help();
                return 1;
        }
    }

    ops = argv + optind;
    nops = argc - optind;

    if(opt_target_dir != NULL && opt_no_target_dir) {
        fprintf(stderr, "%s: cannot combine --target-directory (-t) with "
                        "--no-target-directory (-T)\n", PROG);
        return 1;
    }

    if(opt_target_dir != NULL) {
        struct stat tst;
        if(nops < 1) {
            fprintf(stderr, "%s: missing file operand\n", PROG);
            try_help();
            return 1;
        }
        if(resolve_abs(opt_target_dir, tdir, sizeof(tdir)) != 0 ||
           stat(tdir, &tst) != 0 || !S_ISDIR(tst.st_mode)) {
            fprintf(stderr, "%s: target '%s' is not a directory\n", PROG, opt_target_dir);
            return 1;
        }
        into_dir = 1; srcs = ops; nsrcs = nops;
    } else if(opt_no_target_dir) {
        if(nops < 1) {
            fprintf(stderr, "%s: missing file operand\n", PROG);
            try_help();
            return 1;
        }
        if(nops < 2) {
            fprintf(stderr, "%s: missing destination file operand after '%s'\n", PROG, ops[0]);
            try_help();
            return 1;
        }
        if(nops > 2) {
            fprintf(stderr, "%s: extra operand '%s'\n", PROG, ops[2]);
            try_help();
            return 1;
        }
        into_dir = 0; src_arg = ops[0]; dst_arg = ops[1];
    } else if(nops < 1) {
        fprintf(stderr, "%s: missing file operand\n", PROG);
        try_help();
        return 1;
    } else if(nops < 2) {
        fprintf(stderr, "%s: missing destination file operand after '%s'\n", PROG, ops[0]);
        try_help();
        return 1;
    } else if(nops == 2) {
        /* DEST acts as a directory only when it already exists as one. */
        struct stat dst;
        char dabs[FS_FULL_NAME_MAX + 1];
        if(resolve_abs(ops[1], dabs, sizeof(dabs)) == 0 &&
           stat(dabs, &dst) == 0 && S_ISDIR(dst.st_mode)) {
            into_dir = 1; strcpy(tdir, dabs); srcs = ops; nsrcs = 1;
        } else {
            into_dir = 0; src_arg = ops[0]; dst_arg = ops[1];
        }
    } else {
        struct stat tst;
        if(resolve_abs(ops[nops - 1], tdir, sizeof(tdir)) != 0 ||
           stat(tdir, &tst) != 0 || !S_ISDIR(tst.st_mode)) {
            fprintf(stderr, "%s: target '%s' is not a directory\n", PROG, ops[nops - 1]);
            return 1;
        }
        into_dir = 1; srcs = ops; nsrcs = nops - 1;
    }

    g_buf = (char*)malloc(CP_BUF_SIZE);
    if(g_buf == NULL) {
        fprintf(stderr, "%s: out of memory\n", PROG);
        return 1;
    }

    if(into_dir) {
        for(i = 0; i < nsrcs; i++)
            copy_into_dir(srcs[i], tdir);
    } else {
        copy_to_path(src_arg, dst_arg);
    }

    free(g_buf);
    return g_status;
}
