#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ewoksys/proc.h>
#include <ewoksys/session.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>

/*
 * cron - the EwokOS scheduled-command daemon, modelled on Linux/Vixie cron.
 *
 * It reads the system crontab /etc/crontab (same layout as Linux):
 *
 *   # comment
 *   NAME=value                       <- environment for later jobs
 *   <min> <hour> <dom> <mon> <dow> <user> <command...>
 *
 * Time fields accept '*', ranges 'a-b', steps '* /n' or 'a-b/n', comma
 * lists, and (for month/dow) the 3-letter names. dow is 0-7 with both 0 and
 * 7 = Sunday. As in classic cron, when BOTH day-of-month and day-of-week are
 * restricted the entry fires when either matches; if one is '*' the other
 * decides.
 *
 * The daemon wakes once per second, and on each new minute it re-reads the
 * crontab (so edits take effect without a restart) and launches every entry
 * whose schedule matches. A job runs the command through $SHELL (default
 * /bin/shell) fed on stdin, so shell syntax (&&, |, >, ...) works, as the
 * user named on the line.
 *
 * Privilege: switching to a job's user needs CAP_ROOT (setuid is gated on
 * it). cron is granted CAP_ROOT through the exec-time policy in /etc/cap.json
 * (see the "/sbin/cron" rule), exactly like /bin/login and /bin/sudo. On a
 * system with no cap.json init runs the whole tree as root, so cron is root
 * and holds CAP_ROOT inherently.
 */

#define CRONTAB_FILE  "/etc/crontab"
#define DEFAULT_SHELL "/bin/shell"
#define DEFAULT_PATH  "/sbin:/bin:/bin/x"

#define MAX_ENTRIES 64
#define MAX_ENV     24
#define CMD_MAX     256
#define NAME_MAX_LEN 64
#define VALUE_MAX_LEN 192

typedef struct {
    uint64_t minute;   /* bits 0..59 */
    uint64_t hour;     /* bits 0..23 */
    uint64_t dom;      /* bits 1..31 */
    uint64_t month;    /* bits 1..12 */
    uint64_t dow;      /* bits 0..6 (7 folded onto 0) */
    bool dom_star;
    bool dow_star;
    char user[SESSION_USER_MAX];
    char command[CMD_MAX];
} cron_entry_t;

typedef struct {
    char name[NAME_MAX_LEN];
    char value[VALUE_MAX_LEN];
} cron_env_t;

static cron_entry_t _entries[MAX_ENTRIES];
static int _entry_num = 0;
static cron_env_t _envs[MAX_ENV];
static int _env_num = 0;
static bool _loaded_once = false;

/* ------------------------------------------------------------------ utils */

static int read_file_buf(const char* path, char* buf, int bufsz) {
    int fd = open(path, O_RDONLY);
    if(fd < 0)
        return -1;
    int total = 0;
    while(total < bufsz - 1) {
        int r = read(fd, buf + total, bufsz - 1 - total);
        if(r <= 0)
            break;
        total += r;
    }
    close(fd);
    buf[total] = 0;
    return total;
}

/* Case-insensitive 3-letter prefix match (tok may be longer, e.g. "january"). */
static int name_match(const char* tok, const char* name) {
    for(int i = 0; i < 3; i++) {
        char a = tok[i];
        char b = name[i];
        if(a >= 'A' && a <= 'Z')
            a = (char)(a + ('a' - 'A'));
        if(a != b)
            return 0;
        if(a == 0)
            break;
    }
    return 1;
}

/* Resolve one value token (name or number) for a field. -1 on error. */
static int field_value(const char* tok, int is_mon, int is_dow) {
    static const char* mons[12] = {
        "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"
    };
    static const char* dows[7] = { "sun","mon","tue","wed","thu","fri","sat" };

    if(is_mon) {
        for(int i = 0; i < 12; i++)
            if(name_match(tok, mons[i]))
                return i + 1;
    }
    if(is_dow) {
        for(int i = 0; i < 7; i++)
            if(name_match(tok, dows[i]))
                return i;
    }
    if(tok[0] < '0' || tok[0] > '9')
        return -1;
    return atoi(tok);
}

/*
 * Parse a comma-separated cron field into a bitmask over [min,max]. *star is
 * set only for a bare '*' (used for the dom/dow OR rule). Returns 0/-1.
 */
static int parse_field(char* f, int min, int max, int is_mon, int is_dow,
                       uint64_t* mask, bool* star) {
    *mask = 0;
    *star = false;

    if(strcmp(f, "*") == 0) {
        *star = true;
        for(int v = min; v <= max; v++)
            *mask |= (1ULL << v);
        return 0;
    }

    char* save = NULL;
    for(char* part = strtok_r(f, ",", &save); part != NULL;
        part = strtok_r(NULL, ",", &save)) {
        int step = 1;
        char* slash = strchr(part, '/');
        if(slash != NULL) {
            *slash = 0;
            step = atoi(slash + 1);
            if(step < 1)
                step = 1;
        }

        int lo, hi;
        if(strcmp(part, "*") == 0) {
            lo = min;
            hi = max;
        }
        else {
            char* dash = strchr(part, '-');
            if(dash != NULL) {
                *dash = 0;
                lo = field_value(part, is_mon, is_dow);
                hi = field_value(dash + 1, is_mon, is_dow);
            }
            else {
                lo = field_value(part, is_mon, is_dow);
                hi = (slash != NULL) ? max : lo;   /* "a/n" means a..max step n */
            }
        }
        if(lo < 0 || hi < 0)
            return -1;
        if(lo < min) lo = min;
        if(hi > max) hi = max;

        if(hi < lo) {   /* wrap-around range, e.g. dow 5-1 */
            for(int v = lo; v <= max; v += step)
                *mask |= (1ULL << v);
            for(int v = min; v <= hi; v += step)
                *mask |= (1ULL << v);
        }
        else {
            for(int v = lo; v <= hi; v += step)
                *mask |= (1ULL << v);
        }
    }
    return (*mask != 0) ? 0 : -1;
}

/* ------------------------------------------------------------- environment */

static void add_env(const char* name, const char* value) {
    if(_env_num >= MAX_ENV)
        return;
    strncpy(_envs[_env_num].name, name, NAME_MAX_LEN - 1);
    _envs[_env_num].name[NAME_MAX_LEN - 1] = 0;
    strncpy(_envs[_env_num].value, value, VALUE_MAX_LEN - 1);
    _envs[_env_num].value[VALUE_MAX_LEN - 1] = 0;
    _env_num++;
}

/* Last definition wins, matching how later crontab lines override earlier. */
static const char* get_envv(const char* name) {
    for(int i = _env_num - 1; i >= 0; i--)
        if(strcmp(_envs[i].name, name) == 0)
            return _envs[i].value;
    return NULL;
}

static void set_default_env(void) {
    _env_num = 0;
    add_env("SHELL", DEFAULT_SHELL);
    add_env("PATH", DEFAULT_PATH);
    add_env("HOME", "/");
}

/* Try to interpret a line as "NAME=value". Returns 1 if it was an env line. */
static int try_parse_env(char* line) {
    char* p = line;
    while(*p != 0 && *p != '=' && *p != ' ' && *p != '\t')
        p++;
    if(*p != '=')
        return 0;
    *p = 0;

    char* name = line;
    if(name[0] == 0)
        return 0;
    for(char* q = name; *q != 0; q++) {
        if(!((*q >= 'A' && *q <= 'Z') || (*q >= 'a' && *q <= 'z') ||
             (*q >= '0' && *q <= '9') || *q == '_'))
            return 0;
    }

    char* value = p + 1;
    while(*value == ' ' || *value == '\t')
        value++;
    int vl = (int)strlen(value);
    while(vl > 0 && (value[vl-1] == '\r' || value[vl-1] == '\n' ||
                     value[vl-1] == ' ' || value[vl-1] == '\t'))
        value[--vl] = 0;
    if(vl >= 2 && ((value[0] == '"' && value[vl-1] == '"') ||
                   (value[0] == '\'' && value[vl-1] == '\''))) {
        value[vl-1] = 0;
        value++;
    }

    add_env(name, value);
    return 1;
}

/* --------------------------------------------------------------- entries */

static int parse_entry(char* line) {
    if(_entry_num >= MAX_ENTRIES)
        return -1;

    char* p = line;
    char* fields[6];
    for(int i = 0; i < 6; i++) {
        while(*p == ' ' || *p == '\t')
            p++;
        if(*p == 0 || *p == '\r' || *p == '\n')
            return -1;   /* not enough fields */
        fields[i] = p;
        while(*p != 0 && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
            p++;
        if(*p != 0) {
            *p = 0;
            p++;
        }
    }

    while(*p == ' ' || *p == '\t')
        p++;
    char* cmd = p;
    int clen = (int)strlen(cmd);
    while(clen > 0 && (cmd[clen-1] == '\n' || cmd[clen-1] == '\r'))
        cmd[--clen] = 0;
    if(clen == 0)
        return -1;   /* no command */

    cron_entry_t* e = &_entries[_entry_num];
    memset(e, 0, sizeof(*e));

    bool dummy;
    if(parse_field(fields[0], 0, 59, 0, 0, &e->minute, &dummy) != 0)
        return -1;
    if(parse_field(fields[1], 0, 23, 0, 0, &e->hour, &dummy) != 0)
        return -1;
    if(parse_field(fields[2], 1, 31, 0, 0, &e->dom, &e->dom_star) != 0)
        return -1;
    if(parse_field(fields[3], 1, 12, 1, 0, &e->month, &dummy) != 0)
        return -1;
    if(parse_field(fields[4], 0, 7, 0, 1, &e->dow, &e->dow_star) != 0)
        return -1;
    if(e->dow & (1ULL << 7)) {   /* 7 == Sunday == 0 */
        e->dow |= 1ULL;
        e->dow &= ~(1ULL << 7);
    }

    strncpy(e->user, fields[5], SESSION_USER_MAX - 1);
    if(clen >= CMD_MAX)
        clen = CMD_MAX - 1;
    memcpy(e->command, cmd, clen);
    e->command[clen] = 0;

    _entry_num++;
    return 0;
}

/* (Re)load /etc/crontab. Returns 0 on success. */
static int load_crontab(void) {
    static char buf[8192];
    int len = read_file_buf(CRONTAB_FILE, buf, sizeof(buf));
    if(len < 0) {
        if(_loaded_once) {
            _entry_num = 0;
            slog("cron: %s disappeared, no jobs scheduled\n", CRONTAB_FILE);
        }
        return -1;
    }

    _entry_num = 0;
    set_default_env();

    int bad = 0;
    char* save = NULL;
    for(char* line = strtok_r(buf, "\n", &save); line != NULL;
        line = strtok_r(NULL, "\n", &save)) {
        char* l = line;
        while(*l == ' ' || *l == '\t')
            l++;
        if(*l == 0 || *l == '#')
            continue;
        if(try_parse_env(l))
            continue;
        if(parse_entry(l) != 0) {
            bad++;
            slog("cron: bad crontab line: %s\n", l);
        }
    }

    if(!_loaded_once) {
        _loaded_once = true;
        slog("cron: loaded %d entr%s from %s (%d bad)\n",
             _entry_num, _entry_num == 1 ? "y" : "ies", CRONTAB_FILE, bad);
    }
    return 0;
}

/* Resolve a program to an absolute path via the given PATH (proc_exec does not
   search PATH itself). */
static int resolve_exec(const char* cmd, char* out, int outsz, const char* pathenv) {
    fsinfo_t info;
    if(cmd[0] == '/') {
        if(vfs_get_by_name(cmd, &info) == 0 && FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
            snprintf(out, outsz, "%s", cmd);
            return 0;
        }
        return -1;
    }

    const char* paths = (pathenv != NULL && pathenv[0] != 0) ? pathenv : DEFAULT_PATH;
    char path[FS_FULL_NAME_MAX] = {0};
    char fname[FS_FULL_NAME_MAX] = {0};
    int i = 0;
    while(1) {
        if(paths[i] == 0 || paths[i] == ':') {
            strncpy(path, paths, i);
            path[i] = 0;
            if(path[0] != 0) {
                snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
                if(vfs_get_by_name(fname, &info) == 0 &&
                        FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
                    snprintf(out, outsz, "%s", fname);
                    return 0;
                }
            }
            if(paths[i] == 0)
                break;
            paths = paths + i + 1;
            i = 0;
            fname[0] = 0;
        }
        ++i;
    }
    return -1;
}

static bool entry_matches(const cron_entry_t* e, const struct tm* t) {
    if((e->minute & (1ULL << t->tm_min)) == 0)
        return false;
    if((e->hour & (1ULL << t->tm_hour)) == 0)
        return false;
    if((e->month & (1ULL << (t->tm_mon + 1))) == 0)
        return false;

    bool dom_ok = (e->dom & (1ULL << t->tm_mday)) != 0;
    bool dow_ok = (e->dow & (1ULL << t->tm_wday)) != 0;

    if(e->dom_star && e->dow_star)
        return true;
    if(e->dom_star)
        return dow_ok;
    if(e->dow_star)
        return dom_ok;
    return dom_ok || dow_ok;   /* both restricted: classic OR semantics */
}

/* ------------------------------------------------------------- job runner */

static void run_entry(const cron_entry_t* e) {
    session_info_t uinfo;
    if(session_get_by_name(e->user, &uinfo) != 0) {
        slog("cron: unknown user '%s', skip: %s\n", e->user, e->command);
        return;
    }

    const char* shell = get_envv("SHELL");
    if(shell == NULL || shell[0] == 0)
        shell = DEFAULT_SHELL;
    char shellpath[FS_FULL_NAME_MAX];
    if(resolve_exec(shell, shellpath, sizeof(shellpath), get_envv("PATH")) != 0) {
        slog("cron: cannot resolve shell '%s', skip: %s\n", shell, e->command);
        return;
    }

    int fds[2] = {-1, -1};
    if(pipe(fds) != 0) {
        slog("cron: pipe failed, skip: %s\n", e->command);
        return;
    }

    slog("cron: run '%s' as %s\n", e->command, e->user);

    int pid = fork();
    if(pid == 0) {
        /* child: stdin = command pipe, stdout/stderr = /dev/null */
        close(fds[1]);
        dup2(fds[0], 0);
        close(fds[0]);

        int devnull = open("/dev/null", O_WRONLY);
        if(devnull >= 0) {
            dup2(devnull, 1);
            dup2(devnull, 2);
            close(devnull);
        }

        for(int i = 0; i < _env_num; i++)
            setenv(_envs[i].name, _envs[i].value);
        setenv("HOME", uinfo.home);
        setenv("USER", uinfo.user);
        setenv("LOGNAME", uinfo.user);
        setenv("CONSOLE_ID", "");   /* force plain (non-telnet) stdin reads */

        if(setgid(uinfo.gid) != 0)
            exit(2);
        if(setuid(uinfo.uid) != 0)
            exit(3);

        proc_detach();   /* fire-and-forget: reparented, no zombie for cron */
        if(proc_exec(shellpath) < 0)
            exit(4);
        exit(0);
    }

    /* parent: feed the command to the shell, then EOF so it exits afterwards */
    close(fds[0]);
    size_t clen = strlen(e->command);
    (void)write(fds[1], e->command, clen);
    (void)write(fds[1], "\n", 1);
    close(fds[1]);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    slog("cron: started\n");

    long last_run = -1;
    while(1) {
        time_t now = time(NULL);
        long cur_min = (long)(now / 60);
        if(cur_min != last_run) {
            last_run = cur_min;
            struct tm t;
            localtime_r(&now, &t);
            if(load_crontab() == 0) {
                for(int i = 0; i < _entry_num; i++) {
                    if(entry_matches(&_entries[i], &t))
                        run_entry(&_entries[i]);
                }
            }
        }
        proc_usleep(1000000);   /* 1s poll; cheap and drift-tolerant */
    }
    return 0;
}
