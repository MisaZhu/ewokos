#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/errno.h>
#include <ewoksys/session.h>
#include <ewoksys/sudod.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vfs.h>
#include <utils/telnet_console.h>

/*
 * sudo - the unprivileged client half.
 *
 * EwokOS has no setuid-root ELF bit; authority is carried by capabilities.
 * Instead of granting /bin/sudo CAP_ROOT (the old monolithic design), the
 * privileged work now lives in the sudod daemon. This client only:
 *
 *   1. resolves the command path using the caller's PATH,
 *   2. sends an exec request to sudod over IPC,
 *   3. prompts for the caller's password when sudod asks for it,
 *   4. relays terminal I/O between the caller and the spawned command over
 *      shm pipes, and reports the command's exit status.
 *
 * Authorization (/etc/sudoers) and authentication (sessiond) happen inside
 * sudod; this process is never trusted and holds no special capability.
 */

#define PIPE_DEV_PATH "/dev/pipe0"

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
            usleep(1000);
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
                usleep(30000);
                continue;
            }
            break;
        }

        c = telnet ? telnet_console_parse(&_telnet_console, (uint8_t)c) : (uint8_t)c;
        if(c == 0)
            continue;

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

/* Send one SUDOD_EXEC request and read the status (plus shm_in on success).
   Returns 0 on a transport success, -1 if sudod could not be reached. */
static int sudod_request(const char* target_user, const char* prog,
                         const char* args, const char* password,
                         int shm_out, int shm_err, int shm_ctl,
                         int* status, int* shm_in) {
    int pid = ipc_serv_get(IPC_SERV_SUDOD);
    if(pid < 0)
        return -1;

    proto_t in, out;
    PF->init(&in)
        ->adds(&in, target_user)
        ->adds(&in, prog)
        ->adds(&in, args)
        ->adds(&in, password)
        ->addi(&in, shm_out)
        ->addi(&in, shm_err)
        ->addi(&in, shm_ctl);
    PF->init(&out);

    int res = ipc_call(pid, SUDOD_EXEC, &in, &out);
    PF->clear(&in);
    if(res != 0) {
        PF->clear(&out);
        return -1;
    }

    *status = proto_read_int(&out);
    if(*status == SUDOD_OK)
        *shm_in = proto_read_int(&out);
    PF->clear(&out);
    return 0;
}

/* Flush everything currently buffered on fd to the given output fd, without
   ever blocking (poll timeout 0). Used just before reading the exit status so
   no trailing output is dropped. */
static void drain(int fd, int to_fd) {
    char b[4096];
    for(;;) {
        struct pollfd p = { fd, POLLIN, 0 };
        if(poll(&p, 1, 0) <= 0)
            return;
        ssize_t n = read(fd, b, sizeof(b));
        if(n > 0) {
            (void)write_all_retry(to_fd, b, (size_t)n);
            continue;
        }
        return;   /* EOF or error */
    }
}

/* Relay terminal I/O to/from the spawned command until sudod's worker reports
   the exit status on the control pipe. */
static int relay_and_wait(int in_w, int out_r, int err_r, int ctl_r) {
    char buf[4096];
    bool stdin_open = true;
    bool out_open = true;
    bool err_open = true;
    int32_t code = -1;

    for(;;) {
        struct pollfd pfds[4];
        int n = 0;
        if(stdin_open) {
            pfds[n].fd = 0; pfds[n].events = POLLIN; pfds[n].revents = 0; n++;
        }
        if(out_open) {
            pfds[n].fd = out_r; pfds[n].events = POLLIN; pfds[n].revents = 0; n++;
        }
        if(err_open) {
            pfds[n].fd = err_r; pfds[n].events = POLLIN; pfds[n].revents = 0; n++;
        }
        pfds[n].fd = ctl_r; pfds[n].events = POLLIN; pfds[n].revents = 0; n++;

        int rc = poll(pfds, n, -1);
        if(rc < 0) {
            if(errno == EINTR)
                continue;
            break;
        }
        if(rc == 0)
            continue;

        int idx = 0;
        if(stdin_open) {
            if(pfds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                ssize_t r = read(0, buf, sizeof(buf));
                if(r > 0) {
                    if(write_all_retry(in_w, buf, (size_t)r) != 0) {
                        /* command closed its stdin */
                        close(in_w);
                        stdin_open = false;
                    }
                }
                else {
                    /* terminal EOF: stop feeding the command */
                    close(in_w);
                    stdin_open = false;
                }
            }
            idx++;
        }
        if(out_open) {
            if(pfds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                ssize_t r = read(out_r, buf, sizeof(buf));
                if(r > 0)
                    (void)write_all_retry(1, buf, (size_t)r);
                else {
                    close(out_r);
                    out_open = false;
                }
            }
            idx++;
        }
        if(err_open) {
            if(pfds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                ssize_t r = read(err_r, buf, sizeof(buf));
                if(r > 0)
                    (void)write_all_retry(2, buf, (size_t)r);
                else {
                    close(err_r);
                    err_open = false;
                }
            }
            idx++;
        }

        /* Control pipe is readable => the command has exited and the worker
           has written the exit status. Flush any trailing output first. */
        if(pfds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
            drain(out_r, 1);
            drain(err_r, 2);
            (void)read(ctl_r, &code, sizeof(code));
            break;
        }
    }

    close(ctl_r);
    return (int)code;
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

    /* Resolve what will run: sudod picks the target shell when no command is
       given; otherwise the caller's PATH context resolves the program path. */
    char prog[FS_FULL_NAME_MAX];
    char args[1024];
    prog[0] = 0;
    args[0] = 0;
    if(cmdind >= argc) {
        /* no command => sudod runs the target user's login shell */
    }
    else {
        if(resolve_exec(argv[cmdind], prog, sizeof(prog)) != 0) {
            fprintf(stderr, "sudo: %s: command not found\n", argv[cmdind]);
            return 1;
        }
        int n = 0;
        for(int i = cmdind + 1; i < argc && n < (int)sizeof(args) - 1; i++)
            n += snprintf(args + n, sizeof(args) - n, "%s%s", (n > 0 ? " " : ""), argv[i]);
    }

    /* Create the stdout / stderr / control pipes; sudod's worker attaches the
       write ends. We own the read ends. */
    int out_r = open(PIPE_DEV_PATH, O_RDONLY);
    int err_r = open(PIPE_DEV_PATH, O_RDONLY);
    int ctl_r = open(PIPE_DEV_PATH, O_RDONLY);
    if(out_r < 0 || err_r < 0 || ctl_r < 0) {
        fprintf(stderr, "sudo: cannot create pipes\n");
        return 1;
    }

    fsinfo_t oi, ei, ci;
    if(vfs_get_by_fd(out_r, &oi) != 0 || vfs_get_by_fd(err_r, &ei) != 0 ||
            vfs_get_by_fd(ctl_r, &ci) != 0) {
        fprintf(stderr, "sudo: cannot resolve pipes\n");
        return 1;
    }
    int shm_out = (int32_t)oi.data;
    int shm_err = (int32_t)ei.data;
    int shm_ctl = (int32_t)ci.data;

    int status = SUDOD_ERR_INTERNAL;
    int shm_in = 0;
    if(sudod_request(target_user, prog, args, "", shm_out, shm_err, shm_ctl,
                     &status, &shm_in) != 0) {
        fprintf(stderr, "sudo: cannot contact sudod (is it running?)\n");
        return 1;
    }

    if(status == SUDOD_NEED_PASSWORD) {
        int uid = getuid();
        session_info_t self;
        const char* who = "user";
        if(session_get_by_uid(uid, &self) == 0)
            who = self.user;

        printf("[sudo] password for %s: ", who);
        fflush(stdout);

        str_t* pw = str_new("");
        input(pw, false);
        printf("\n");

        int rc = sudod_request(target_user, prog, args, pw->cstr, shm_out, shm_err,
                               shm_ctl, &status, &shm_in);
        str_free(pw);
        if(rc != 0) {
            fprintf(stderr, "sudo: cannot contact sudod (is it running?)\n");
            return 1;
        }
    }

    switch(status) {
    case SUDOD_ERR_DENY:
        fprintf(stderr, "sudo: %s is not allowed to run '%s' as %s (see /etc/sudoers)\n",
                getenv("USER") ? getenv("USER") : "you", prog, target_user);
        return 1;
    case SUDOD_ERR_PASSWORD:
        fprintf(stderr, "Sorry, incorrect password.\n");
        return 1;
    case SUDOD_ERR_USER:
        fprintf(stderr, "sudo: unknown user: %s\n", target_user);
        return 1;
    case SUDOD_OK:
        break;
    default:
        fprintf(stderr, "sudo: sudod error (%d)\n", status);
        return 1;
    }

    /* Attach the write end of sudod's stdin pipe and start relaying. */
    int in_w = -1;
    if(vfs_pipe_attach(shm_in, &in_w) != 0) {
        fprintf(stderr, "sudo: cannot attach stdin\n");
        return 1;
    }

    return relay_and_wait(in_w, out_r, err_r, ctl_r);
}
