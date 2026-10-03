#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/session.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vfs.h>
#include <ewoksys/cap.h>
#include <utils/telnet_console.h>

/*
 * sudo - run a command (or the target user's login shell) with another
 * user's identity, root by default.
 *
 * EwokOS has no setuid-root ELF bit; authority is carried by capabilities.
 * /bin/sudo is granted CAP_ROOT through the exec-time policy in
 * /etc/cap.json (see machine.virt/system/etc/basic/cap.json), exactly like
 * /bin/login. That CAP_ROOT is what lets this process call setgid()/setuid()
 * to switch identity even when it was started by an ordinary user. Once the
 * uid is switched, proc_exec() replaces this image with the target command;
 * the kernel re-derives capabilities from the new uid at exec time, so a
 * switch to uid 0 yields a fully privileged command and a switch to a normal
 * user yields an unprivileged one.
 *
 * When no /etc/cap.json is present, init runs the whole boot/session tree as
 * root (uid 0); the kernel grants CAP_ROOT to any image exec'd while uid<=0,
 * so a root-invoked sudo still works with no policy at all. A non-root caller
 * on such a system has no way to obtain CAP_ROOT, and sudo reports that
 * clearly instead of failing inside setuid().
 *
 * Authorization is driven by /etc/sudoers (see system/basic/etc/sudoers):
 * a non-root caller must be listed there for the requested target user and
 * command. root is always allowed and never consults the file.
 *
 * Authentication mirrors login: the invoking user's OWN password is verified
 * through sessiond (session_check) - sudo proves who you are, not that you
 * know the target's password. It is skipped for root, when already the target
 * identity, or when the matching rule carries NOPASSWD.
 */

static telnet_console_t _telnet_console;

static int write_all_retry(int fd, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t off = 0;
    while(off < len) {
        ssize_t wr = write(fd, p + off, len - off);
        if(wr > 0) {
            off += (size_t)wr;
            continue;
        }
        if(errno == EAGAIN || errno == EINTR) {
            proc_usleep(1000);
            continue;
        }
        if(wr == 0 && errno == 0)
            errno = EPIPE;
        return -1;
    }
    return 0;
}

/* Read one line from the console/tty, optionally masking the echo. Shared
   with login so that password entry behaves identically on the local console
   and over a telnet session. */
static void input(str_t* s, bool show) {
    bool telnet = telnet_console_is_active();

    str_reset(s);
    char c;
    while(true) {
        int i = telnet ? telnet_console_read(0, &_telnet_console, &c) : read(0, &c, 1);
        if(i <= 0) {
            int err = errno;
            if(err == EAGAIN || err == EINTR) {
                proc_usleep(30000);
                continue;
            }
            break;
        }

        c = telnet ? telnet_console_parse(&_telnet_console, (uint8_t)c) : (uint8_t)c;
        if(c == 0) {
            continue;
        }

        if(!telnet && c == '\r')
            c = '\n';

        if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
            if(s->len > 0) {
                if(show) {
                    static const char erase_seq[3] = { CONSOLE_LEFT, ' ', CONSOLE_LEFT };
                    (void)write_all_retry(1, erase_seq, sizeof(erase_seq));
                }
                s->len--;
            }
        }
        else {
            char out = c;
            if(!show && c != '\n')
                out = '*';

            (void)write_all_retry(1, &out, 1);

            if(c == '\n')
                break;

            if(c > 27)
                str_addc(s, c);
        }
    }
}

static void usage(void) {
    printf("usage: sudo [-u user] [-h] [command [args...]]\n");
}

/* Resolve a command name to a full path the way the shell does: absolute and
   "./" forms are used as-is, otherwise every PATH directory is probed. This
   is needed because proc_exec() does not search PATH itself. */
static int resolve_exec(const char* cmd, char* out, int outsz) {
    fsinfo_t info;

    if(cmd[0] == '/') {
        if(vfs_get_by_name(cmd, &info) == 0 && FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
            snprintf(out, outsz, "%s", cmd);
            return 0;
        }
        return -1;
    }

    if(cmd[0] == '.' && cmd[1] == '/') {
        char cwd[FS_FULL_NAME_MAX+1];
        const char* path = getcwd(cwd, FS_FULL_NAME_MAX);
        char fname[FS_FULL_NAME_MAX];
        snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd+2);
        if(vfs_get_by_name(fname, &info) == 0 && FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
            snprintf(out, outsz, "%s", fname);
            return 0;
        }
        return -1;
    }

    const char* paths = getenv("PATH");
    if(paths == NULL || paths[0] == 0)
        paths = "/sbin:/bin:/bin/x";

    char path[FS_FULL_NAME_MAX] = {0};
    char fname[FS_FULL_NAME_MAX] = {0};
    int i = 0;
    while(1) {
        if(paths[i] == 0 || paths[i] == ':') {
            strncpy(path, paths, i);
            path[i] = 0;
            if(path[0] != 0) {
                snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
                if(vfs_get_by_name(fname, &info) == 0 && FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
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

/* True when this process holds CAP_ROOT, the authority setuid()/setgid() are
   gated on. A process exec'd while uid<=0 is granted CAP_ROOT by the kernel,
   so a root caller always passes here - with or without /etc/cap.json. A
   non-root caller only holds it when the exec-time policy (cap.json) granted
   the /bin/sudo rule. Mirrors the scan captest does. */
static bool have_root_cap(void) {
    cap_info_t info;
    for(int32_t i = 0; i < CNODE_SLOTS; i++) {
        if(cap_get(i, &info) == 0 && info.cap.type == CAP_ROOT)
            return true;
    }
    return false;
}

/* Prompt for and verify the invoking user's password via sessiond. */
static int authenticate(const char* user) {
    printf("[sudo] password for %s: ", user);
    fflush(stdout);

    str_t* pw = str_new("");
    input(pw, false);
    printf("\n");

    session_info_t info;
    int ok = (session_check(user, pw->cstr, &info) == 0);
    str_free(pw);
    return ok;
}

/* ----------------------------------------------------------------------
 * /etc/sudoers authorization.
 *
 * A simplified, recognizable subset of the classic sudoers grammar:
 *
 *   # comment
 *   <spec> <host>=(<runas>) [NOPASSWD:] <command>[,<command>...]
 *
 *   <spec>   : user name, "%group" (name or gid), or ALL
 *   <host>   : ignored (single-host system), conventionally ALL
 *   <runas>  : target user, "user:group" (group ignored), or ALL
 *   <command>: ALL, or absolute paths (comma separated)
 *
 * The last matching rule decides, mirroring real sudo. Root (uid 0) never
 * consults this file - it is allowed unconditionally. When the file is
 * absent a non-root caller is denied (secure default).
 * -------------------------------------------------------------------- */
#define SUDOERS_FILE "/etc/sudoers"

#define SUDO_DENY     (-1)
#define SUDO_PASSWD    0
#define SUDO_NOPASSWD  1

/* Read a small text file fully into buf. Returns the length, or -1. */
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

/* Match a "%group" spec (the part after '%') against the caller's gid. */
static bool group_matches(const char* spec, int gid) {
    if(spec[0] >= '0' && spec[0] <= '9')
        return atoi(spec) == gid;
    session_group_t g;
    if(session_get_group_by_gid(gid, &g) == 0 && strcmp(g.group, spec) == 0)
        return true;
    return false;
}

static bool spec_matches(const char* spec, const char* user, int gid) {
    if(strcmp(spec, "ALL") == 0)
        return true;
    if(spec[0] == '%')
        return group_matches(spec + 1, gid);
    return strcmp(spec, user) == 0;
}

/* Parse "HOST=(RUNAS)" and report whether RUNAS permits target_user. A rule
   with no "(...)" clause is treated as unrestricted. */
static bool runas_matches(const char* hostspec, const char* target_user) {
    const char* lp = strchr(hostspec, '(');
    const char* rp = strchr(hostspec, ')');
    char runas[SESSION_USER_MAX];

    if(lp != NULL && rp != NULL && rp > lp) {
        const char* p = lp + 1;
        int i = 0;
        while(p < rp && i < (int)sizeof(runas) - 1) {
            if(*p == ':')   /* "user:group" - only the user part matters */
                break;
            runas[i++] = *p++;
        }
        runas[i] = 0;
    }
    else {
        strcpy(runas, "ALL");
    }

    if(runas[0] == 0 || strcmp(runas, "ALL") == 0)
        return true;
    return strcmp(runas, target_user) == 0;
}

/* Match a comma-separated command list against cmd. "ALL" matches anything. */
static bool cmd_matches(const char* list, const char* cmd) {
    if(strcmp(list, "ALL") == 0)
        return true;
    if(cmd == NULL)
        return false;
    const char* p = list;
    while(*p != 0) {
        const char* comma = strchr(p, ',');
        int len = (comma != NULL) ? (int)(comma - p) : (int)strlen(p);
        if(len > 0 && strncmp(p, cmd, len) == 0 && cmd[len] == 0)
            return true;
        if(comma == NULL)
            break;
        p = comma + 1;
    }
    return false;
}

/* Evaluate one sudoers line. Returns SUDO_PASSWD / SUDO_NOPASSWD when the
   rule grants the request, or SUDO_DENY when it does not apply. */
static int parse_rule(char* line, const char* user, int gid,
                      const char* target_user, const char* cmd) {
    char* hash = strchr(line, '#');
    if(hash != NULL)
        *hash = 0;

    char* toks[16];
    int n = 0;
    char* save = NULL;
    for(char* t = strtok_r(line, " \t\r\n", &save); t != NULL && n < 16;
        t = strtok_r(NULL, " \t\r\n", &save))
        toks[n++] = t;

    if(n < 2 || strcmp(toks[0], "Defaults") == 0)
        return SUDO_DENY;
    if(!spec_matches(toks[0], user, gid))
        return SUDO_DENY;
    if(!runas_matches(toks[1], target_user))
        return SUDO_DENY;

    int nopasswd = 0;
    bool cmd_ok = (n == 2);   /* "spec host=(runas)" with no command => ALL */
    for(int i = 2; i < n; i++) {
        char* t = toks[i];
        if(strncmp(t, "NOPASSWD:", 9) == 0) {
            nopasswd = 1;
            t += 9;
            if(*t == 0)
                continue;
        }
        else if(strncmp(t, "PASSWD:", 7) == 0) {
            nopasswd = 0;
            t += 7;
            if(*t == 0)
                continue;
        }
        if(cmd_matches(t, cmd))
            cmd_ok = true;
    }

    if(!cmd_ok)
        return SUDO_DENY;
    return nopasswd ? SUDO_NOPASSWD : SUDO_PASSWD;
}

/* Decide whether `user` (primary gid `gid`) may run `cmd` as `target_user`.
   Returns SUDO_PASSWD, SUDO_NOPASSWD, or SUDO_DENY. */
static int sudoers_authorize(const char* user, int gid,
                             const char* target_user, const char* cmd) {
    static char buf[4096];
    if(read_file_buf(SUDOERS_FILE, buf, sizeof(buf)) < 0)
        return SUDO_DENY;   /* no policy file: deny non-root callers */

    int result = SUDO_DENY;
    char* save = NULL;
    for(char* line = strtok_r(buf, "\n", &save); line != NULL;
        line = strtok_r(NULL, "\n", &save)) {
        int r = parse_rule(line, user, gid, target_user, cmd);
        if(r != SUDO_DENY)
            result = r;   /* last matching rule wins */
    }
    return result;
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    telnet_console_init(&_telnet_console);

    const char* target_user = "root";
    int c;
    while((c = getopt(argc, argv, "u:h")) != -1) {
        switch(c) {
        case 'u':
            target_user = optarg;
            break;
        case 'h':
            usage();
            return 0;
        default:
            usage();
            return 1;
        }
    }
    int cmdind = optind;

    session_info_t target;
    if(session_get_by_name(target_user, &target) != 0) {
        fprintf(stderr, "sudo: unknown user: %s\n", target_user);
        return 1;
    }

    int uid = getuid();
    int gid = getgid();

    /*
     * Resolve what we are going to run BEFORE switching identity: the caller's
     * PATH context is used for the lookup, and the resolved program path also
     * feeds the /etc/sudoers command match. With no command given we run the
     * target user's login shell.
     */
    char prog[FS_FULL_NAME_MAX];
    char cmdline[1024];
    if(cmdind >= argc) {
        const char* sh = target.cmd;
        if(sh[0] == 0)
            sh = "/bin/shell";
        snprintf(prog, sizeof(prog), "%s", sh);
        snprintf(cmdline, sizeof(cmdline), "%s", sh);
    }
    else {
        if(resolve_exec(argv[cmdind], prog, sizeof(prog)) != 0) {
            fprintf(stderr, "sudo: %s: command not found\n", argv[cmdind]);
            return 1;
        }
        int n = snprintf(cmdline, sizeof(cmdline), "%s", prog);
        for(int i = cmdind+1; i < argc && n > 0 && n < (int)sizeof(cmdline)-1; i++)
            n += snprintf(cmdline+n, sizeof(cmdline)-n, " %s", argv[i]);
    }

    /*
     * Switching identity needs CAP_ROOT: setuid()/setgid() are gated on it.
     * When the caller already runs as the target identity there is nothing to
     * switch and no authority is required. Otherwise:
     *  - a root caller (uid<=0) is granted CAP_ROOT by the kernel at exec, so
     *    this works even on a system with no /etc/cap.json (there init runs the
     *    whole tree as root);
     *  - a non-root caller only holds CAP_ROOT when the cap.json exec policy
     *    granted the /bin/sudo rule. Without either, escalation is impossible
     *    by design, so fail early with an actionable message instead of a
     *    cryptic setuid error.
     */
    if(uid != target.uid || gid != target.gid) {
        if(!have_root_cap()) {
            fprintf(stderr,
                "sudo: cannot escalate to '%s': this /bin/sudo holds no CAP_ROOT.\n"
                "      Run it as root, or on a system using a capability policy\n"
                "      grant /bin/sudo the root capability in /etc/cap.json:\n"
                "        { \"cmd\": \"/bin/sudo\", \"caps\": [ {\"type\":\"root\",\"rights\":\"rwxg\"} ] }\n",
                target.user);
            return 1;
        }

        /* Root is trusted outright; anyone else must be authorized by
           /etc/sudoers for this target user and command, then (unless the rule
           is NOPASSWD) prove their own identity with their password. */
        if(uid != 0) {
            session_info_t self;
            if(session_get_by_uid(uid, &self) != 0) {
                fprintf(stderr, "sudo: cannot resolve current user (uid %d)\n", uid);
                return 1;
            }
            int auth = sudoers_authorize(self.user, gid, target.user, prog);
            if(auth == SUDO_DENY) {
                fprintf(stderr,
                    "sudo: %s is not allowed to run '%s' as %s (see /etc/sudoers)\n",
                    self.user, prog, target.user);
                return 1;
            }
            if(auth == SUDO_PASSWD && !authenticate(self.user)) {
                fprintf(stderr, "Sorry, incorrect password.\n");
                return 1;
            }
        }

        /* gid before uid: setuid to a real user (uid>0) drops our caps. */
        if(gid != target.gid && setgid(target.gid) != 0) {
            fprintf(stderr, "sudo: setgid(%d) failed\n", target.gid);
            return 1;
        }
        if(uid != target.uid && setuid(target.uid) != 0) {
            fprintf(stderr, "sudo: setuid(%d) failed\n", target.uid);
            return 1;
        }
        setenv("USER", target.user);
        setenv("HOME", target.home);
    }

    /* On success this replaces our image and never returns. */
    if(proc_exec(cmdline) < 0)
        fprintf(stderr, "sudo: cannot exec %s\n", cmdline);
    return 1;
}

