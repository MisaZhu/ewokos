#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/session.h>
#include <ewoksys/sudod.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/proto.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>

/*
 * sudod - the privileged half of sudo.
 *
 * A long-lived daemon that runs with CAP_ROOT (granted through the exec-time
 * policy in /etc/cap.json, exactly like the old monolithic /bin/sudo did).
 * The unprivileged /bin/sudo client sends an exec request over IPC; sudod is
 * the only process trusted to:
 *
 *   - derive the caller's uid/gid from the IPC sender (never from the client),
 *   - authorize the request against /etc/sudoers, and
 *   - authenticate the caller's own password through sessiond.
 *
 * On success sudod forks a worker that attaches write ends to the stdout /
 * stderr / control pipes the client owns, then forks the actual command which
 * setgid()/setuid()s to the target user and execs. The client relays terminal
 * I/O over these shm pipes and reads the command's exit status from the
 * control pipe. sudod itself never blocks on the command and stays in its IPC
 * loop; the detached worker is reaped by init when it finishes.
 */

/*
 * Minimal wire constants for the standalone pipe driver. libc keeps no
 * piped-specific header (see vfs.c); sudod only needs these two values.
 */
#define PIPE_DEV_PATH "/dev/pipe0"

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

/* ----------------------------------------------------------------------
 * Command spawning.
 *
 * The fork tree is:
 *
 *   sudod (IPC loop)
 *     `- worker  (detached; attaches out/err/ctl write ends, waits, reports)
 *          `- command  (dup2 stdio, setgid/setuid, proc_exec)
 *
 * The stdin pipe read end is created by sudod itself and inherited by the
 * command through both forks; the client attaches the write end once sudod
 * replies with its shm id. stdout/stderr/control pipes are owned (read end)
 * by the client, so the worker attaches the write ends via vfs_pipe_attach().
 * -------------------------------------------------------------------- */

/*
 * The IPC handler runs on sudod's main context, which the kernel hijacks to
 * serve single-task requests. That context must never fork() or block, so
 * the handler only authorizes the request and queues the actual spawn; the
 * main loop below performs the fork from a clean (non-hijacked) context.
 */
#define SUDOD_JOB_MAX 32

typedef struct {
    int in_r;
    int shm_out;
    int shm_err;
    int shm_ctl;
    session_info_t target;
    char cmdline[1024];
    int valid;
} sudod_job_t;

static sudod_job_t _jobs[SUDOD_JOB_MAX];

static int sudod_job_alloc(void) {
    for(int i = 0; i < SUDOD_JOB_MAX; i++) {
        if(!_jobs[i].valid)
            return i;
    }
    return -1;
}

/* Report a spawn failure on the client's stderr pipe so sudo never fails
   silently; the worker still sends the exit status on the control pipe. */
static void worker_error(int err_w, const char* msg) {
    if(err_w >= 0)
        (void)write(err_w, msg, strlen(msg));
}

static void spawn_worker(int in_r, session_info_t* target, const char* cmdline,
                         int shm_out, int shm_err, int shm_ctl) {
    /* Reparent to init so sudod never accumulates our zombie. */
    proc_detach();

    int out_w = -1, err_w = -1, ctl_w = -1;
    int32_t fail = -1;
    if(vfs_pipe_attach(shm_out, &out_w) != 0 ||
            vfs_pipe_attach(shm_err, &err_w) != 0 ||
            vfs_pipe_attach(shm_ctl, &ctl_w) != 0) {
        worker_error(err_w, "sudo: cannot attach output pipes\n");
        if(ctl_w >= 0)
            (void)write(ctl_w, &fail, sizeof(fail));
        close(in_r);
        if(out_w >= 0) close(out_w);
        if(err_w >= 0) close(err_w);
        if(ctl_w >= 0) close(ctl_w);
        exit(-1);
    }

    int cpid = fork();
    if(cpid < 0) {
        worker_error(err_w, "sudo: fork failed\n");
        (void)write(ctl_w, &fail, sizeof(fail));
        close(in_r); close(out_w); close(err_w); close(ctl_w);
        exit(-1);
    }

    if(cpid == 0) {
        /* Command: wire stdio to the client's pipes, switch identity, exec. */
        if(dup2(in_r, 0) < 0 || dup2(out_w, 1) < 0 || dup2(err_w, 2) < 0) {
            worker_error(err_w, "sudo: cannot wire command stdio\n");
            exit(126);
        }
        close(in_r);
        close(out_w);
        close(err_w);
        close(ctl_w);

        /* gid before uid: setuid to a real user (uid>0) drops our caps. */
        if(setgid(target->gid) != 0 || setuid(target->uid) != 0) {
            char msg[128];
            snprintf(msg, sizeof(msg), "sudo: cannot switch to user %s: %s\n",
                     target->user, strerror(errno));
            worker_error(2, msg);
            exit(126);
        }

        setenv("USER", target->user);
        setenv("HOME", target->home);
        proc_exec(cmdline);   /* replaces this image; returns only on failure */
        char msg[FS_FULL_NAME_MAX + 128];
        snprintf(msg, sizeof(msg), "sudo: unable to execute %s: %s\n",
                 cmdline, errno != 0 ? strerror(errno) : "exec failed");
        worker_error(2, msg);
        exit(127);
    }

    /* Worker: drop the ends the command owns, wait, report exit status. */
    close(in_r);
    close(out_w);
    close(err_w);

    int code = ewok_waitpid(cpid);
    int32_t e = (int32_t)code;
    (void)write(ctl_w, &e, sizeof(e));
    close(ctl_w);
    exit(0);
}

static void do_sudod_exec(int from_pid, proto_t* in, proto_t* out) {
    /* Trust the kernel's view of who is talking to us, never the payload. */
    int cpid = proc_getpid(from_pid);
    procinfo_t pinfo;
    if(proc_info(cpid, &pinfo) != 0) {
        PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
        return;
    }
    int caller_uid = pinfo.uid;
    int caller_gid = pinfo.gid;

    const char* target_user = proto_read_str(in);
    const char* prog = proto_read_str(in);
    const char* args = proto_read_str(in);
    const char* password = proto_read_str(in);
    int shm_out = proto_read_int(in);
    int shm_err = proto_read_int(in);
    int shm_ctl = proto_read_int(in);

    if(target_user == NULL || shm_out <= 0 || shm_err <= 0 || shm_ctl <= 0) {
        PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
        return;
    }

    session_info_t target;
    if(session_get_by_name(target_user, &target) != 0) {
        PF->clear(out)->addi(out, SUDOD_ERR_USER);
        return;
    }

    /* Resolve what we run: an explicit path (validated to exist), or the
       target user's login shell when no command was given. */
    char resolved[FS_FULL_NAME_MAX];
    if(prog == NULL || prog[0] == 0) {
        const char* sh = target.cmd;
        if(sh[0] == 0)
            sh = "/bin/shell";
        snprintf(resolved, sizeof(resolved), "%s", sh);
    }
    else {
        fsinfo_t info;
        if(vfs_get_by_name(prog, &info) != 0 || !FS_IS_TYPE(info.type, FS_TYPE_FILE)) {
            PF->clear(out)->addi(out, SUDOD_ERR_DENY);
            return;
        }
        snprintf(resolved, sizeof(resolved), "%s", prog);
    }

    /* Authorization + authentication (root is trusted outright). */
    if(caller_uid != 0) {
        session_info_t self;
        if(session_get_by_uid(caller_uid, &self) != 0) {
            PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
            return;
        }
        int auth = sudoers_authorize(self.user, caller_gid, target.user, resolved);
        if(auth == SUDO_DENY) {
            PF->clear(out)->addi(out, SUDOD_ERR_DENY);
            return;
        }
        if(auth == SUDO_PASSWD) {
            if(password == NULL || password[0] == 0) {
                PF->clear(out)->addi(out, SUDOD_NEED_PASSWORD);
                return;
            }
            session_info_t tmp;
            if(session_check(self.user, password, &tmp) != 0) {
                PF->clear(out)->addi(out, SUDOD_ERR_PASSWORD);
                return;
            }
        }
    }

    char cmdline[1024];
    if(args == NULL || args[0] == 0)
        snprintf(cmdline, sizeof(cmdline), "%s", resolved);
    else
        snprintf(cmdline, sizeof(cmdline), "%s %s", resolved, args);

    /* Create the stdin pipe (we own the read end, the command inherits it). */
    int in_r = open(PIPE_DEV_PATH, O_RDONLY);
    if(in_r < 0) {
        PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
        return;
    }
    fsinfo_t in_info;
    if(vfs_get_by_fd(in_r, &in_info) != 0 || (int32_t)in_info.data <= 0) {
        close(in_r);
        PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
        return;
    }
    int shm_in = (int32_t)in_info.data;

    int job = sudod_job_alloc();
    if(job < 0) {
        close(in_r);
        PF->clear(out)->addi(out, SUDOD_ERR_INTERNAL);
        return;
    }

    sudod_job_t* j = &_jobs[job];
    j->in_r = in_r;
    j->shm_out = shm_out;
    j->shm_err = shm_err;
    j->shm_ctl = shm_ctl;
    j->target = target;
    strncpy(j->cmdline, cmdline, sizeof(j->cmdline) - 1);
    j->cmdline[sizeof(j->cmdline) - 1] = 0;
    j->valid = 1;

    /* The command's stdin pipe (shm_in) is handed to the client; the queued
       job owns the read end until the main loop forks the worker. */
    PF->clear(out)->addi(out, SUDOD_OK)->addi(out, shm_in);
}

static void handle_ipc(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
    switch(cmd) {
    case SUDOD_EXEC:
        do_sudod_exec(from_pid, in, out);
        return;
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if(ipc_serv_reg(IPC_SERV_SUDOD) != 0) {
        slog("sudod: register IPC service failed\n");
        return -1;
    }

    ipc_serv_run(handle_ipc, NULL, NULL, 0);

    while(1) {
        for(int i = 0; i < SUDOD_JOB_MAX; i++) {
            sudod_job_t* j = &_jobs[i];
            if(!j->valid)
                continue;

            int rpid = fork();
            if(rpid < 0)
                continue;   /* leave queued, retry next pass */
            if(rpid == 0) {
                spawn_worker(j->in_r, &j->target, j->cmdline,
                             j->shm_out, j->shm_err, j->shm_ctl);
                exit(-1);   /* not reached */
            }
            close(j->in_r);
            j->valid = 0;
        }
        usleep(10000);
    }
    return 0;
}
