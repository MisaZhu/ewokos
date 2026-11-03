#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <ewoksys/session.h>

/* ─── File entry ─── */
typedef struct {
    char name[FS_NODE_NAME_MAX];
    char path[FS_FULL_NAME_MAX];
    struct stat st;
    int is_link;       /* lstat says symlink */
    int stat_ok;       /* stat/lstat succeeded */
} file_entry_t;

/* ─── Sort orders ─── */
enum { SORT_NAME = 0, SORT_SIZE, SORT_TIME, SORT_EXT, SORT_NONE };

/* ─── Output formats ─── */
enum { FMT_COLUMNS = 0, FMT_LONG, FMT_ONE, FMT_COMMAS, FMT_ACROSS };

/* ─── Color mode ─── */
enum { COLOR_AUTO = 0, COLOR_ALWAYS, COLOR_NEVER };

/* ─── Global options ─── */
static int opt_all = 0;           /* -a */
static int opt_almost_all = 0;    /* -A */
static int opt_long = 0;          /* -l */
static int opt_human = 0;         /* -h */
static int opt_inode = 0;         /* -i */
static int opt_classify = 0;      /* -F */
static int opt_dir_slash = 0;     /* -p */
static int opt_recursive = 0;     /* -R */
static int opt_reverse = 0;       /* -r */
static int opt_deref = 0;         /* -L */
static int opt_numeric = 0;       /* -n */
static int opt_no_group = 0;      /* -g */
static int opt_no_owner = 0;      /* -o */
static int opt_dir_entry = 0;     /* -d */
static int opt_alloc_size = 0;    /* -s */
static int opt_full_time = 0;     /* --full-time */
static int opt_kbytes = 0;        /* -k */
static int opt_block_size = 0;    /* --block-size */
static int sort_mode = SORT_NAME;
static int time_field = 0;        /* 0=mtime, 1=ctime, 2=atime */
static int fmt_mode = FMT_COLUMNS;
static int color_mode = COLOR_AUTO;
static int use_color = 0;

static int term_width = 80;

/* ─── ANSI color codes ─── */
#define CLR_DIR     "\033[01;34m"
#define CLR_LNK     "\033[01;36m"
#define CLR_SOCK    "\033[01;35m"
#define CLR_FIFO    "\033[33m"
#define CLR_BLK     "\033[01;33m"
#define CLR_CHR     "\033[01;33m"
#define CLR_EXEC    "\033[01;32m"
#define CLR_RESET   "\033[0m"
#define CLR_BOLD    "\033[1m"

static const char* get_color(const file_entry_t* e) {
    if(e->is_link && !opt_deref)
        return CLR_LNK;
    mode_t m = e->st.st_mode;
    if(S_ISDIR(m))   return CLR_DIR;
    if(S_ISLNK(m))   return CLR_LNK;
    if(S_ISSOCK(m))  return CLR_SOCK;
    if(S_ISFIFO(m))  return CLR_FIFO;
    if(S_ISBLK(m))   return CLR_BLK;
    if(S_ISCHR(m))   return CLR_CHR;
    if(m & (S_IXUSR | S_IXGRP | S_IXOTH))
        return CLR_EXEC;
    return NULL;
}

static char classify_char(const file_entry_t* e) {
    mode_t m = e->st.st_mode;
    if(S_ISDIR(m))   return '/';
    if(S_ISLNK(m))   return '@';
    if(S_ISSOCK(m))  return '=';
    if(S_ISFIFO(m))  return '|';
    if(m & (S_IXUSR | S_IXGRP | S_IXOTH))
        return '*';
    return 0;
}

/* ─── Human-readable size ─── */
static void format_size_human(off_t size, char* buf, size_t bufsz) {
    const char* units[] = {"", "K", "M", "G", "T", "P"};
    double d = (double)size;
    int ui = 0;
    while(d >= 1024.0 && ui < 5) {
        d /= 1024.0;
        ui++;
    }
    if(ui == 0)
        snprintf(buf, bufsz, "%lld", (long long)size);
    else if(d < 10.0)
        snprintf(buf, bufsz, "%.1f%s", d, units[ui]);
    else
        snprintf(buf, bufsz, "%.0f%s", d, units[ui]);
}

/* ─── Block size (for -s, and size column in -l) ─── */
static long long get_block_count(const file_entry_t* e) {
    long long sz;
    if(e->is_link && !opt_deref)
        sz = (long long)e->st.st_size;
    else
        sz = (long long)e->st.st_blocks * 512;

    if(opt_kbytes)
        return (sz + 1023) / 1024;
    if(opt_block_size > 0)
        return (sz + opt_block_size - 1) / opt_block_size;
    if(opt_human)
        return sz; /* handled separately */
    return (sz + 1023) / 1024; /* default: 1K blocks like GNU ls */
}

static void format_block_size(const file_entry_t* e, char* buf, size_t bufsz) {
    long long sz;
    if(e->is_link && !opt_deref)
        sz = (long long)e->st.st_size;
    else
        sz = (long long)e->st.st_blocks * 512;

    if(opt_human) {
        format_size_human((off_t)sz, buf, bufsz);
    } else if(opt_kbytes) {
        snprintf(buf, bufsz, "%lld", (sz + 1023) / 1024);
    } else if(opt_block_size > 0) {
        snprintf(buf, bufsz, "%lld", (sz + opt_block_size - 1) / opt_block_size);
    } else {
        snprintf(buf, bufsz, "%lld", (sz + 1023) / 1024);
    }
}

/* ─── Permission string ─── */
static void format_mode(mode_t mode, int is_link, char* buf) {
    char t;
    if(is_link && !opt_deref) t = 'l';
    else if(S_ISDIR(mode))    t = 'd';
    else if(S_ISCHR(mode))    t = 'c';
    else if(S_ISBLK(mode))    t = 'b';
    else if(S_ISFIFO(mode))   t = 'p';
    else if(S_ISLNK(mode))    t = 'l';
    else if(S_ISSOCK(mode))   t = 's';
    else                      t = '-';

    buf[0] = t;
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? ((mode & S_ISUID) ? 's' : 'x')
                               : ((mode & S_ISUID) ? 'S' : '-');
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? ((mode & S_ISGID) ? 's' : 'x')
                               : ((mode & S_ISGID) ? 'S' : '-');
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? ((mode & S_ISVTX) ? 't' : 'x')
                               : ((mode & S_ISVTX) ? 'T' : '-');
    buf[10] = '\0';
}

/* ─── Date formatting ─── */
static const char* month_names[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

static void format_time(time_t t, char* buf, size_t bufsz) {
    struct tm tm;
    localtime_r(&t, &tm);

    if(opt_full_time) {
        snprintf(buf, bufsz, "%04d-%02d-%02d %02d:%02d:%02d.000000000 +0000",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
        return;
    }

    /* If file is older than 6 months or in the future, show year */
    time_t now = time(NULL);
    long long diff = (long long)(now - t);
    if(diff > 182 * 24 * 3600 || diff < -182 * 24 * 3600) {
        snprintf(buf, bufsz, "%s %2d  %d",
                month_names[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900);
    } else {
        snprintf(buf, bufsz, "%s %2d %02d:%02d",
                month_names[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min);
    }
}

/* ─── Sorting ─── */
static int cmp_name(const void* a, const void* b) {
    const file_entry_t* ea = (const file_entry_t*)a;
    const file_entry_t* eb = (const file_entry_t*)b;
    int r = strcmp(ea->name, eb->name);
    return opt_reverse ? -r : r;
}

static int cmp_size(const void* a, const void* b) {
    const file_entry_t* ea = (const file_entry_t*)a;
    const file_entry_t* eb = (const file_entry_t*)b;
    int r;
    if(ea->st.st_size < eb->st.st_size) r = 1;
    else if(ea->st.st_size > eb->st.st_size) r = -1;
    else r = strcmp(ea->name, eb->name);
    return opt_reverse ? -r : r;
}

static int cmp_time(const void* a, const void* b) {
    const file_entry_t* ea = (const file_entry_t*)a;
    const file_entry_t* eb = (const file_entry_t*)b;
    time_t ta, tb;
    switch(time_field) {
        case 1: ta = ea->st.st_ctime; tb = eb->st.st_ctime; break;
        case 2: ta = ea->st.st_atime; tb = eb->st.st_atime; break;
        default: ta = ea->st.st_mtime; tb = eb->st.st_mtime; break;
    }
    int r;
    if(ta < tb) r = 1;
    else if(ta > tb) r = -1;
    else r = strcmp(ea->name, eb->name);
    return opt_reverse ? -r : r;
}

static int cmp_ext(const void* a, const void* b) {
    const file_entry_t* ea = (const file_entry_t*)a;
    const file_entry_t* eb = (const file_entry_t*)b;
    const char* da = strrchr(ea->name, '.');
    const char* db = strrchr(eb->name, '.');
    if(!da) da = "";
    if(!db) db = "";
    int r = strcmp(da, db);
    if(r == 0) r = strcmp(ea->name, eb->name);
    return opt_reverse ? -r : r;
}

static void sort_entries(file_entry_t* entries, int count) {
    if(count <= 1 || sort_mode == SORT_NONE)
        return;
    switch(sort_mode) {
        case SORT_SIZE: qsort(entries, count, sizeof(file_entry_t), cmp_size); break;
        case SORT_TIME: qsort(entries, count, sizeof(file_entry_t), cmp_time); break;
        case SORT_EXT:  qsort(entries, count, sizeof(file_entry_t), cmp_ext); break;
        default:        qsort(entries, count, sizeof(file_entry_t), cmp_name); break;
    }
}

/* ─── Join path helper ─── */
static void join_path(char* out, size_t outsz, const char* dir, const char* name) {
    size_t len = strlen(dir);
    if(len > 0 && dir[len-1] == '/')
        snprintf(out, outsz, "%s%s", dir, name);
    else
        snprintf(out, outsz, "%s/%s", dir, name);
}

/* ─── Read directory entries ─── */
static file_entry_t* read_dir_entries(const char* dir, int* count) {
    *count = 0;
    DIR* dp = opendir(dir);
    if(!dp) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", dir, strerror(errno));
        return NULL;
    }

    int cap = 64;
    file_entry_t* entries = (file_entry_t*)malloc(cap * sizeof(file_entry_t));
    if(!entries) { closedir(dp); return NULL; }

    struct dirent* de;
    while((de = readdir(dp)) != NULL) {
        const char* nm = de->d_name;

        /* Skip . and .. unless -a */
        if(nm[0] == '.') {
            if(nm[1] == '\0' || (nm[1] == '.' && nm[2] == '\0')) {
                if(!opt_all) continue;
            } else {
                if(!opt_all && !opt_almost_all) continue;
            }
        }

        if(*count >= cap) {
            cap *= 2;
            file_entry_t* tmp = (file_entry_t*)realloc(entries, cap * sizeof(file_entry_t));
            if(!tmp) break;
            entries = tmp;
        }

        file_entry_t* e = &entries[*count];
        memset(e, 0, sizeof(*e));
        strncpy(e->name, nm, FS_NODE_NAME_MAX - 1);
        join_path(e->path, FS_FULL_NAME_MAX, dir, nm);

        int r;
        if(opt_deref)
            r = stat(e->path, &e->st);
        else
            r = lstat(e->path, &e->st);

        if(r == 0) {
            e->stat_ok = 1;
            e->is_link = S_ISLNK(e->st.st_mode) ? 1 : 0;
            if(e->is_link && opt_deref) {
                /* already dereferenced via stat() */
                e->is_link = 0;
            }
        } else {
            e->stat_ok = 0;
        }
        (*count)++;
    }
    closedir(dp);
    return entries;
}

/* ─── Print long format for one entry ─── */
static void print_long_entry(const file_entry_t* e) {
    char modebuf[12];
    char timebuf[64];
    char sizebuf[32];
    char owner[SESSION_USER_MAX];
    char group[SESSION_GROUP_MAX];

    if(!e->stat_ok) {
        printf("?????????? ? ? ? ? ? %s\n", e->name);
        return;
    }

    format_mode(e->st.st_mode, e->is_link, modebuf);

    time_t t;
    switch(time_field) {
        case 1: t = e->st.st_ctime; break;
        case 2: t = e->st.st_atime; break;
        default: t = e->st.st_mtime; break;
    }
    format_time(t, timebuf, sizeof(timebuf));

    if(opt_human)
        format_size_human(e->st.st_size, sizebuf, sizeof(sizebuf));
    else
        snprintf(sizebuf, sizeof(sizebuf), "%lld", (long long)e->st.st_size);

    /* Owner */
    if(!opt_no_owner) {
        if(opt_numeric)
            snprintf(owner, sizeof(owner), "%d", e->st.st_uid);
        else {
            session_info_t si;
            if(session_get_by_uid(e->st.st_uid, &si) == 0)
                strncpy(owner, si.user, SESSION_USER_MAX - 1);
            else
                snprintf(owner, sizeof(owner), "%d", e->st.st_uid);
        }
        owner[sizeof(owner) - 1] = '\0';
    }

    /* Group */
    if(!opt_no_group) {
        if(opt_numeric)
            snprintf(group, sizeof(group), "%d", e->st.st_gid);
        else {
            session_group_t gi;
            if(session_get_group_by_gid(e->st.st_gid, &gi) == 0)
                strncpy(group, gi.group, SESSION_GROUP_MAX - 1);
            else
                snprintf(group, sizeof(group), "%d", e->st.st_gid);
        }
        group[sizeof(group) - 1] = '\0';
    }

    /* Inode */
    if(opt_inode)
        printf("%7llu ", (unsigned long long)e->st.st_ino);

    /* Allocated size */
    if(opt_alloc_size) {
        char blkbuf[32];
        format_block_size(e, blkbuf, sizeof(blkbuf));
        printf("%5s ", blkbuf);
    }

    /* Mode + links */
    printf("%s %2llu ", modebuf, (unsigned long long)e->st.st_nlink);

    /* Owner */
    if(!opt_no_owner)
        printf("%-8s ", owner);

    /* Group */
    if(!opt_no_group)
        printf("%-8s ", group);

    /* Device major,minor for block/char */
    if(S_ISBLK(e->st.st_mode) || S_ISCHR(e->st.st_mode)) {
        printf("%3u, %3u ", (unsigned)(e->st.st_rdev >> 8) & 0xff,
               (unsigned)(e->st.st_rdev & 0xff));
    } else {
        printf("%8s ", sizebuf);
    }

    /* Date */
    printf("%s ", timebuf);

    /* Name with color and classify */
    if(use_color) {
        const char* c = get_color(e);
        if(c) printf("%s%s%s", c, e->name, CLR_RESET);
        else  printf("%s", e->name);
    } else {
        printf("%s", e->name);
    }

    if(opt_classify || opt_dir_slash) {
        char cc = classify_char(e);
        if(opt_dir_slash && cc != '/') cc = 0;
        if(cc) putchar(cc);
    }

    /* Symlink target */
    if(e->is_link && !opt_deref) {
        char linkbuf[FS_FULL_NAME_MAX];
        ssize_t len = readlink(e->path, linkbuf, FS_FULL_NAME_MAX - 1);
        if(len > 0) {
            linkbuf[len] = '\0';
            printf(" -> %s", linkbuf);
        }
    }

    printf("\n");
}

/* ─── Get display string for an entry (with classify/color prefix) ─── */
static int get_display_name(const file_entry_t* e, char* buf, size_t bufsz) {
    int len = 0;

    if(use_color) {
        const char* c = get_color(e);
        if(c) {
            len = snprintf(buf, bufsz, "%s%s%s", c, e->name, CLR_RESET);
        } else {
            len = snprintf(buf, bufsz, "%s", e->name);
        }
    } else {
        len = snprintf(buf, bufsz, "%s", e->name);
    }

    if((opt_classify || opt_dir_slash) && len < (int)bufsz - 2) {
        char cc = classify_char(e);
        if(opt_dir_slash && cc != '/') cc = 0;
        if(cc) {
            buf[len++] = cc;
            buf[len] = '\0';
        }
    }

    return len;
}

/* ─── Plain name length (without ANSI escape codes) ─── */
static int plain_name_len(const file_entry_t* e) {
    int len = (int)strlen(e->name);
    if(opt_classify || opt_dir_slash) {
        char cc = classify_char(e);
        if(opt_dir_slash && cc != '/') cc = 0;
        if(cc) len++;
    }
    if(opt_inode && e->stat_ok)
        len += 8; /* "NNNNNNN " */
    return len;
}

/* ─── Print in columns (default or -C/-x) ─── */
static void print_columns(file_entry_t* entries, int count) {
    if(count == 0) return;

    /* Calculate column width */
    int maxw = 0;
    for(int i = 0; i < count; i++) {
        int w = plain_name_len(&entries[i]);
        if(w > maxw) maxw = w;
    }
    maxw += 2; /* padding */

    int cols = term_width / maxw;
    if(cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for(int r = 0; r < rows; r++) {
        for(int c = 0; c < cols; c++) {
            int idx;
            if(fmt_mode == FMT_ACROSS)
                idx = r * cols + c;
            else
                idx = c * rows + r;

            if(idx >= count) break;

            char disp[256];
            get_display_name(&entries[idx], disp, sizeof(disp));

            if(opt_inode && entries[idx].stat_ok)
                printf("%7llu ", (unsigned long long)entries[idx].st.st_ino);

            int plen = plain_name_len(&entries[idx]);
            printf("%s", disp);

            /* Pad to column width unless last column */
            if(c < cols - 1) {
                int pad = maxw - plen;
                for(int p = 0; p < pad; p++) putchar(' ');
            }
        }
        printf("\n");
    }
}

/* ─── Print comma-separated (-m) ─── */
static void print_commas(file_entry_t* entries, int count) {
    for(int i = 0; i < count; i++) {
        char disp[256];
        get_display_name(&entries[i], disp, sizeof(disp));
        if(opt_inode && entries[i].stat_ok)
            printf("%7llu ", (unsigned long long)entries[i].st.st_ino);
        printf("%s", disp);
        if(i < count - 1) printf(", ");
    }
    printf("\n");
}

/* ─── List a single directory ─── */
static void list_dir(const char* dir, int show_header, int* total_blocks) {
    int count = 0;
    file_entry_t* entries = read_dir_entries(dir, &count);
    if(!entries) return;

    sort_entries(entries, count);

    /* Calculate total blocks for "total" line */
    long long blocks = 0;
    for(int i = 0; i < count; i++) {
        if(entries[i].stat_ok) {
            if(entries[i].is_link && !opt_deref)
                blocks += 0; /* symlinks don't count */
            else
                blocks += (entries[i].st.st_blocks + 1) / 2; /* 512-byte to 1K */
        }
    }

    if(show_header)
        printf("%s:\n", dir);

    if(opt_long || fmt_mode == FMT_ONE) {
        if(opt_long && count > 0)
            printf("total %lld\n", blocks);
        if(total_blocks) *total_blocks += (int)blocks;

        for(int i = 0; i < count; i++) {
            if(opt_long)
                print_long_entry(&entries[i]);
            else {
                if(opt_inode && entries[i].stat_ok)
                    printf("%7llu ", (unsigned long long)entries[i].st.st_ino);
                if(opt_alloc_size) {
                    char blkbuf[32];
                    format_block_size(&entries[i], blkbuf, sizeof(blkbuf));
                    printf("%5s ", blkbuf);
                }
                char disp[256];
                get_display_name(&entries[i], disp, sizeof(disp));
                printf("%s\n", disp);
            }
        }
    } else if(fmt_mode == FMT_COMMAS) {
        print_commas(entries, count);
    } else {
        print_columns(entries, count);
    }

    /* Recurse into subdirectories */
    if(opt_recursive) {
        for(int i = 0; i < count; i++) {
            if(!entries[i].stat_ok) continue;
            if(S_ISDIR(entries[i].st.st_mode)) {
                /* Skip . and .. */
                if(strcmp(entries[i].name, ".") == 0 || strcmp(entries[i].name, "..") == 0)
                    continue;
                printf("\n");
                list_dir(entries[i].path, 1, total_blocks);
            }
        }
    }

    free(entries);
}

/* ─── List a single file (not a directory) ─── */
static void list_file(const char* path) {
    file_entry_t e;
    memset(&e, 0, sizeof(e));
    strncpy(e.path, path, FS_FULL_NAME_MAX - 1);

    /* Extract basename */
    const char* slash = strrchr(path, '/');
    strncpy(e.name, slash ? slash + 1 : path, FS_NODE_NAME_MAX - 1);

    int r;
    if(opt_deref)
        r = stat(path, &e.st);
    else
        r = lstat(path, &e.st);

    if(r != 0) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }
    e.stat_ok = 1;
    e.is_link = S_ISLNK(e.st.st_mode) ? 1 : 0;

    if(opt_long) {
        printf("total %lld\n", (long long)(e.st.st_blocks + 1) / 2);
        print_long_entry(&e);
    } else if(fmt_mode == FMT_ONE || fmt_mode == FMT_COLUMNS) {
        if(opt_inode)
            printf("%7llu ", (unsigned long long)e.st.st_ino);
        if(opt_alloc_size) {
            char blkbuf[32];
            format_block_size(&e, blkbuf, sizeof(blkbuf));
            printf("%5s ", blkbuf);
        }
        char disp[256];
        get_display_name(&e, disp, sizeof(disp));
        printf("%s\n", disp);
    } else if(fmt_mode == FMT_COMMAS) {
        char disp[256];
        get_display_name(&e, disp, sizeof(disp));
        printf("%s\n", disp);
    }
}

/* ─── Usage ─── */
static void usage(void) {
    printf("Usage: ls [OPTION]... [FILE]...\n");
    printf("List information about the FILEs (the current directory by default).\n\n");
    printf("  -a, --all                  do not ignore entries starting with .\n");
    printf("  -A, --almost-all           do not list implied . and ..\n");
    printf("  -c                         sort by ctime; with -l show ctime\n");
    printf("  -C                         list entries by columns (default)\n");
    printf("  -d, --directory            list directories themselves, not contents\n");
    printf("  -F, --classify             append indicator to entries\n");
    printf("  -g                         like -l, but do not list owner\n");
    printf("  -G, --no-group             in long listing, don't print group names\n");
    printf("  -h, --human-readable       with -l, print sizes in human readable format\n");
    printf("  -i, --inode                print the index number of each file\n");
    printf("  -k, --kibibytes            use 1024-byte blocks\n");
    printf("  -l                         use a long listing format\n");
    printf("  -L, --dereference          follow symbolic links\n");
    printf("  -m                         fill width with a comma separated list\n");
    printf("  -n, --numeric-uid-gid      like -l, but list numeric user and group IDs\n");
    printf("  -o                         like -l, but do not list group information\n");
    printf("  -p                         append / indicator to directories\n");
    printf("  -r, --reverse              reverse order while sorting\n");
    printf("  -R, --recursive            list subdirectories recursively\n");
    printf("  -s, --size                 print the allocated size of each file\n");
    printf("  -S                         sort by file size, largest first\n");
    printf("  -t                         sort by modification time, newest first\n");
    printf("  -u                         with -t: sort by access time; with -l: show atime\n");
    printf("  -U                         do not sort; list entries in directory order\n");
    printf("  -x                         list entries by lines instead of by columns\n");
    printf("  -X                         sort alphabetically by entry extension\n");
    printf("  -1                         list one file per line\n");
    printf("      --color[=WHEN]         colorize output (auto, always, never)\n");
    printf("      --full-time            like -l --time-style=full-iso\n");
    printf("      --block-size=SIZE      scale sizes by SIZE\n");
    printf("      --help                 display this help and exit\n");
}

/* ─── Long options ─── */
static struct option long_options[] = {
    {"all",             no_argument,       0, 'a'},
    {"almost-all",      no_argument,       0, 'A'},
    {"classify",        no_argument,       0, 'F'},
    {"color",           optional_argument, 0,  1 },
    {"directory",       no_argument,       0, 'd'},
    {"dereference",     no_argument,       0, 'L'},
    {"full-time",       no_argument,       0,  2 },
    {"human-readable",  no_argument,       0, 'h'},
    {"inode",           no_argument,       0, 'i'},
    {"kibibytes",       no_argument,       0, 'k'},
    {"no-group",        no_argument,       0, 'G'},
    {"numeric-uid-gid", no_argument,       0, 'n'},
    {"recursive",       no_argument,       0, 'R'},
    {"reverse",         no_argument,       0, 'r'},
    {"size",            no_argument,       0, 's'},
    {"block-size",      required_argument, 0,  3 },
    {"help",            no_argument,       0,  4 },
    {0, 0, 0, 0}
};

/* ─── Parse block-size argument ─── */
static int parse_block_size(const char* arg) {
    char* end;
    long val = strtol(arg, &end, 10);
    if(val <= 0) return 0;
    if(*end) {
        switch(*end) {
            case 'K': case 'k': val *= 1024; break;
            case 'M': case 'm': val *= 1024*1024; break;
            case 'G': case 'g': val *= 1024*1024*1024; break;
        }
    }
    return (int)val;
}

/* ─── Option parsing ─── */
static int doargs(int argc, char* argv[]) {
    int c;
    while((c = getopt_long(argc, argv, "aAcCdFgGhiklLmnoPrRsStuUxX1",
                    long_options, NULL)) != -1) {
        switch(c) {
            case 'a': opt_all = 1; break;
            case 'A': opt_almost_all = 1; break;
            case 'c': time_field = 1; sort_mode = SORT_TIME; break;
            case 'C': fmt_mode = FMT_COLUMNS; break;
            case 'd': opt_dir_entry = 1; break;
            case 'F': opt_classify = 1; break;
            case 'g': opt_long = 1; opt_no_owner = 1; break;
            case 'G': opt_no_group = 1; break;
            case 'h': opt_human = 1; break;
            case 'i': opt_inode = 1; break;
            case 'k': opt_kbytes = 1; break;
            case 'l': opt_long = 1; break;
            case 'L': opt_deref = 1; break;
            case 'm': fmt_mode = FMT_COMMAS; break;
            case 'n': opt_long = 1; opt_numeric = 1; break;
            case 'o': opt_long = 1; opt_no_group = 1; break;
            case 'P': opt_deref = 0; break;
            case 'r': opt_reverse = 1; break;
            case 'R': opt_recursive = 1; break;
            case 's': opt_alloc_size = 1; break;
            case 'S': sort_mode = SORT_SIZE; break;
            case 't': sort_mode = SORT_TIME; break;
            case 'u': time_field = 2; if(sort_mode == SORT_TIME) break; sort_mode = SORT_TIME; break;
            case 'U': sort_mode = SORT_NONE; break;
            case 'x': fmt_mode = FMT_ACROSS; break;
            case 'X': sort_mode = SORT_EXT; break;
            case '1': fmt_mode = FMT_ONE; break;
            case 1: /* --color */
                if(!optarg || strcmp(optarg, "auto") == 0)
                    color_mode = COLOR_AUTO;
                else if(strcmp(optarg, "always") == 0)
                    color_mode = COLOR_ALWAYS;
                else if(strcmp(optarg, "never") == 0)
                    color_mode = COLOR_NEVER;
                break;
            case 2: /* --full-time */
                opt_full_time = 1;
                opt_long = 1;
                break;
            case 3: /* --block-size */
                opt_block_size = parse_block_size(optarg);
                break;
            case 4: /* --help */
                usage();
                exit(0);
            case '?':
            default:
                fprintf(stderr, "Try 'ls --help' for more information.\n");
                exit(2);
        }
    }

    /* -n implies -l */
    if(opt_numeric) opt_long = 1;

    /* If -l is set, override column format */
    if(opt_long && fmt_mode == FMT_COLUMNS)
        fmt_mode = FMT_LONG;
    if(opt_long) fmt_mode = FMT_LONG;

    /* Determine color usage */
    switch(color_mode) {
        case COLOR_ALWAYS: use_color = 1; break;
        case COLOR_NEVER:  use_color = 0; break;
        case COLOR_AUTO:   use_color = isatty(STDOUT_FILENO) ? 1 : 0; break;
    }

    /* Check CLICOLOR_FORCE / NO_COLOR environment */
    if(color_mode == COLOR_AUTO) {
        const char* nc = getenv("NO_COLOR");
        if(nc && nc[0]) use_color = 0;
        const char* cf = getenv("CLICOLOR_FORCE");
        if(cf && cf[0] && strcmp(cf, "0") != 0) use_color = 1;
    }

    /* Get terminal width */
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;
    else {
        const char* cols_env = getenv("COLUMNS");
        if(cols_env) {
            int v = atoi(cols_env);
            if(v > 0) term_width = v;
        }
    }

    return optind;
}

int main(int argc, char* argv[]) {
    int argind = doargs(argc, argv);

    /* No arguments: list current directory */
    if(argind >= argc) {
        char cwd[FS_FULL_NAME_MAX];
        if(getcwd(cwd, FS_FULL_NAME_MAX) == NULL) {
            fprintf(stderr, "ls: cannot get working directory: %s\n", strerror(errno));
            return 1;
        }
        int dummy = 0;
        list_dir(cwd, 0, &dummy);
        return 0;
    }

    /* Collect arguments into files and directories */
    int nargs = argc - argind;
    char** files = (char**)malloc(nargs * sizeof(char*));
    char** dirs = (char**)malloc(nargs * sizeof(char*));
    int nfiles = 0, ndirs = 0;

    for(int i = argind; i < argc; i++) {
        struct stat st;
        const char* path = argv[i];
        int r = opt_deref ? stat(path, &st) : lstat(path, &st);
        if(r != 0) {
            fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
            continue;
        }
        if(S_ISDIR(st.st_mode) && !opt_dir_entry)
            dirs[ndirs++] = argv[i];
        else
            files[nfiles++] = argv[i];
    }

    /* List non-directory files first */
    if(nfiles > 0) {
        if(nfiles == 1) {
            list_file(files[0]);
        } else {
            /* Build entry array for sorting */
            file_entry_t* entries = (file_entry_t*)calloc(nfiles, sizeof(file_entry_t));
            int cnt = 0;
            for(int i = 0; i < nfiles; i++) {
                file_entry_t* e = &entries[cnt];
                strncpy(e->path, files[i], FS_FULL_NAME_MAX - 1);
                const char* slash = strrchr(files[i], '/');
                strncpy(e->name, slash ? slash + 1 : files[i], FS_NODE_NAME_MAX - 1);
                int r = opt_deref ? stat(files[i], &e->st) : lstat(files[i], &e->st);
                if(r == 0) {
                    e->stat_ok = 1;
                    e->is_link = S_ISLNK(e->st.st_mode) ? 1 : 0;
                }
                cnt++;
            }
            sort_entries(entries, cnt);

            if(fmt_mode == FMT_LONG) {
                long long blocks = 0;
                for(int i = 0; i < cnt; i++)
                    if(entries[i].stat_ok) blocks += (entries[i].st.st_blocks + 1) / 2;
                printf("total %lld\n", blocks);
                for(int i = 0; i < cnt; i++)
                    print_long_entry(&entries[i]);
            } else if(fmt_mode == FMT_ONE) {
                for(int i = 0; i < cnt; i++) {
                    if(opt_inode && entries[i].stat_ok)
                        printf("%7llu ", (unsigned long long)entries[i].st.st_ino);
                    char disp[256];
                    get_display_name(&entries[i], disp, sizeof(disp));
                    printf("%s\n", disp);
                }
            } else if(fmt_mode == FMT_COMMAS) {
                print_commas(entries, cnt);
            } else {
                print_columns(entries, cnt);
            }
            free(entries);
        }
    }

    /* List directories */
    for(int i = 0; i < ndirs; i++) {
        int show_hdr = (nfiles > 0 || ndirs > 1) ? 1 : 0;
        if(show_hdr && (i > 0 || nfiles > 0))
            printf("\n");
        int dummy = 0;
        list_dir(dirs[i], show_hdr, &dummy);
    }

    free(files);
    free(dirs);
    return 0;
}
