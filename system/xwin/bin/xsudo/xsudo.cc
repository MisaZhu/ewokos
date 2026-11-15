#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/errno.h>
#include <string>

#include <ewoksys/sudod.h>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/proto.h>
#include <ewoksys/vfs.h>

#include <x++/X.h>
#include <x++/XTheme.h>
#include <WidgetEx/Dialog.h>
#include <Widget/RootWidget.h>
#include <Widget/Label.h>
#include <Widget/EditLine.h>
#include <Widget/RoundLabelButton.h>
#include <Widget/WidgetX.h>

using namespace Ewok;

/*
 * xsudo - the graphical half of sudo.
 *
 * Mirrors /bin/sudo (see system/basic/bin/sudo/sudo.c) but replaces the
 * terminal password prompt with a widget++ confirmation dialog. The dialog
 * shows the complete command that will run and collects the password in a
 * masked input box. Authorization (/etc/sudoers) and authentication
 * (sessiond) still happen inside the privileged sudod daemon; this process
 * never holds any special capability.
 */

#define PIPE_DEV_PATH "/dev/pipe0"

/* Compact dialog layout (logical units, scaled by X::hidpiSize). */
#define LINE_H 22   /* command message line  */
#define PWD_H  20   /* "Password:" caption   */
#define EDIT_H 32   /* password input box    */
#define ERR_H  18   /* error/status line     */
#define BTN_H  26   /* OK/Cancel row         */
#define DLG_PAD 24  /* horizontal padding    */

/* Everything the password dialog needs to authenticate through sudod. */
struct SudoCtx {
    std::string target_user;
    std::string prog;
    std::string args;
    int shm_out, shm_err, shm_ctl;
    int shm_in;       /* stdin pipe id, filled once sudod accepts us */
    int status;       /* last SUDOD_* status */
};

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

static void usage(void) {
    printf("usage: xsudo [-u user] [-h] [command [args...]]\n");
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
   ever blocking (poll timeout 0). */
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
                        close(in_w);
                        stdin_open = false;
                    }
                }
                else {
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

/* An EditLine that renders its content as '*' so the password is never shown. */
class PasswordEdit : public EditLine {
protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        font_t* font = theme->getFont();
        if(font == NULL)
            return;

        uint32_t bgColor = theme->basic.bgColor;
        uint32_t fgColor = theme->basic.fgColor;
        if(!focused()) {
            bgColor = theme->basic.bgDisableColor;
            fgColor = theme->basic.fgDisableColor;
        }

        std::string masked;
        for(uint32_t i = 0; i < content.length(); i++)
            masked += '*';

        graph_fill_3d(g, r.x, r.y, r.w, r.h, bgColor, true);
        graph_draw_text_font_align(g, r.x+marginH, r.y + (r.h-theme->basic.fontSize)/2,
                    r.w-marginH*2, r.h,
                    masked.c_str(), font, theme->basic.fontSize, fgColor, FONT_ALIGN_NONE);

        if(focused() && showCur) {
            int32_t cw;
            font_text_size(masked.c_str(), font, theme->basic.fontSize, (uint32_t*)&cw, NULL);
            graph_fill_rect(g, r.x+marginH + cw + 2, r.y + (r.h-theme->basic.fontSize)/2,
                        4, theme->basic.fontSize, fgColor);
        }
    }
};

/* A ConfirmDialog-style modal: the message is the full command, plus a masked
   password input. Authentication runs on OK; a wrong password just re-arms
   the input in place instead of closing the dialog. */
class PasswordDialog : public Dialog {
public:
    SudoCtx* ctx;
    std::string info;
    std::string error;
    EditLine* edit;
    Label* errLabel;

    PasswordDialog(SudoCtx* c, const std::string& msg) {
        ctx = c;
        info = msg;
        error = "";
        edit = NULL;
        errLabel = NULL;
    }

protected:
    void onBuild();
    void onPopup() {
        if(root != NULL)
            root->focus(edit);
        callXIM(true);
    }
};

/* Run the password collected in the dialog through sudod and either accept,
   report a wrong password, or reset on error. Shared by the OK button and the
   Enter key in the password field. */
static void submitOk(PasswordDialog* dialog) {
    std::string pw = dialog->edit->getContent();

    int status = SUDOD_ERR_INTERNAL;
    int shm_in = 0;
    if(sudod_request(dialog->ctx->target_user.c_str(), dialog->ctx->prog.c_str(),
                     dialog->ctx->args.c_str(), pw.c_str(),
                     dialog->ctx->shm_out, dialog->ctx->shm_err, dialog->ctx->shm_ctl,
                     &status, &shm_in) != 0) {
        dialog->ctx->status = SUDOD_ERR_INTERNAL;
        dialog->submit(Dialog::RES_RESET);
        return;
    }

    if(status == SUDOD_OK) {
        dialog->ctx->shm_in = shm_in;
        dialog->ctx->status = status;
        dialog->submit(Dialog::RES_OK);
    }
    else if(status == SUDOD_ERR_PASSWORD) {
        dialog->error = "Sorry, incorrect password.";
        dialog->edit->setContent("");
        if(dialog->errLabel != NULL)
            dialog->errLabel->setLabel(dialog->error);
    }
    else {
        dialog->ctx->status = status;
        dialog->submit(Dialog::RES_RESET);
    }
}

static void okFunc(Widget* wd, xevent_t* evt, void* arg) {
    (void)arg;
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    PasswordDialog* dialog = (PasswordDialog*)wd->getWin();
    submitOk(dialog);
}

/* EditLine input callback: pressing Enter in the password box is equivalent
   to clicking OK. */
static void enterFunc(Widget* wd, uint32_t input, void* arg) {
    (void)arg;
    if(input != KEY_ENTER)
        return;
    PasswordDialog* dialog = (PasswordDialog*)wd->getWin();
    submitOk(dialog);
}

static void cancelFunc(Widget* wd, xevent_t* evt, void* arg) {
    (void)arg;
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    PasswordDialog* dialog = (PasswordDialog*)wd->getWin();
    dialog->submit(Dialog::RES_CANCEL);
}

void PasswordDialog::onBuild() {
    RootWidget* root = new RootWidget();
    setRoot(root);
    root->setType(Container::VERTICAL);
    root->setAlpha(false);

    Label* label = new Label("");
    label->setID(1);
    label->fix(0, X::hidpiSize(LINE_H));
    root->add(label);
    label->setLabel(info);

    Label* pwdLabel = new Label("Password:");
    pwdLabel->fix(0, X::hidpiSize(PWD_H));
    root->add(pwdLabel);

    edit = new PasswordEdit();
    edit->setOnInputFunc(enterFunc, NULL);
    edit->fix(0, X::hidpiSize(EDIT_H));
    root->add(edit);

    errLabel = new Label("");
    errLabel->setID(2);
    errLabel->fix(0, X::hidpiSize(ERR_H));
    root->add(errLabel);
    errLabel->setLabel(error);

    Container* c = new Container();
    c->setType(Container::HORIZONTAL);
    c->fix(0, X::hidpiSize(BTN_H));
    root->add(c);

    RoundLabelButton* okButton = new RoundLabelButton("OK");
    okButton->setEventFunc(okFunc);
    c->add(okButton);

    RoundLabelButton* cancelButton = new RoundLabelButton("Cancel");
    cancelButton->setEventFunc(cancelFunc);
    c->add(cancelButton);
}

/* The owner window that hosts the modal password dialog. */
class XSudoWin : public XWin {
public:
    enum Result { R_NONE, R_ACCEPT, R_CANCEL, R_ERROR };

    SudoCtx ctx;
    Result result;
    PasswordDialog dialog;

    XSudoWin(const std::string& t, const std::string& p, const std::string& a,
             int so, int se, int sc, const std::string& info)
        : result(R_NONE), dialog(&ctx, info) {
        ctx.target_user = t;
        ctx.prog = p;
        ctx.args = a;
        ctx.shm_out = so;
        ctx.shm_err = se;
        ctx.shm_ctl = sc;
        ctx.shm_in = 0;
        ctx.status = SUDOD_NEED_PASSWORD;
    }

    void showDialog() {
        xscreen_info_t scr;
        X::getScreenInfo(scr, getDisplayIndex());

        /* Fit the window to the command text so the full command stays visible,
           clamped to the desktop with a compact vertical extent. */
        uint32_t text_w = 0;
        XTheme* th = dialog.getTheme();
        font_t* font = th != NULL ? th->getFont() : NULL;
        if(font != NULL)
            font_text_size(dialog.info.c_str(), font, th->basic.fontSize, &text_w, NULL);

        int w = (int)text_w + X::hidpiSize(DLG_PAD);
        int min_w = X::hidpiSize(240);
        int max_w = (int)scr.size.w - X::hidpiSize(16);
        if(w < min_w) w = min_w;
        if(w > max_w) w = max_w;

        int h = X::hidpiSize(LINE_H + PWD_H + EDIT_H + ERR_H + BTN_H);

        dialog.popup(this, 0, 0, "xsudo", XWIN_STYLE_NO_RESIZE);
        dialog.resizeTo(w, h);
        dialog.moveTo((scr.size.w - w)/2, (scr.size.h - h)/2);
    }

protected:
    void onRepaint(graph_t* g) {
        (void)g;
    }

    void onDialoged(XWin* from, int res, void* arg) {
        (void)from;
        (void)arg;
        switch(res) {
        case Dialog::RES_OK:
            result = R_ACCEPT;
            break;
        case Dialog::RES_RESET:
            result = R_ERROR;
            break;
        default:
            result = R_CANCEL;
            break;
        }
        x->terminate();
    }
};

static void print_sudod_error(int status, const char* target_user, const char* prog) {
    switch(status) {
    case SUDOD_ERR_DENY:
        fprintf(stderr, "xsudo: %s is not allowed to run '%s' as %s (see /etc/sudoers)\n",
                getenv("USER") ? getenv("USER") : "you", prog, target_user);
        break;
    case SUDOD_ERR_PASSWORD:
        fprintf(stderr, "Sorry, incorrect password.\n");
        break;
    case SUDOD_ERR_USER:
        fprintf(stderr, "xsudo: unknown user: %s\n", target_user);
        break;
    default:
        fprintf(stderr, "xsudo: sudod error (%d)\n", status);
        break;
    }
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    std::string target_user = "root";
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

    /* Resolve what will run. sudod picks the target shell when no command is
       given; otherwise the caller's PATH resolves the program path. */
    char prog[FS_FULL_NAME_MAX];
    char args[1024];
    prog[0] = 0;
    args[0] = 0;
    if(cmdind < argc) {
        if(resolve_exec(argv[cmdind], prog, sizeof(prog)) != 0) {
            fprintf(stderr, "xsudo: %s: command not found\n", argv[cmdind]);
            return 1;
        }
        int n = 0;
        for(int i = cmdind + 1; i < argc && n < (int)sizeof(args) - 1; i++)
            n += snprintf(args + n, sizeof(args) - n, "%s%s", (n > 0 ? " " : ""), argv[i]);
    }

    /* Build the complete command string shown in the dialog. */
    std::string commandInfo;
    if(prog[0] == 0)
        commandInfo = "(login shell as " + target_user + ")";
    else if(args[0] == 0)
        commandInfo = prog;
    else
        commandInfo = std::string(prog) + " " + args;

    /* Create the stdout / stderr / control pipes. */
    int out_r = open(PIPE_DEV_PATH, O_RDONLY);
    int err_r = open(PIPE_DEV_PATH, O_RDONLY);
    int ctl_r = open(PIPE_DEV_PATH, O_RDONLY);
    if(out_r < 0 || err_r < 0 || ctl_r < 0) {
        fprintf(stderr, "xsudo: cannot create pipes\n");
        return 1;
    }

    fsinfo_t oi, ei, ci;
    if(vfs_get_by_fd(out_r, &oi) != 0 || vfs_get_by_fd(err_r, &ei) != 0 ||
            vfs_get_by_fd(ctl_r, &ci) != 0) {
        fprintf(stderr, "xsudo: cannot resolve pipes\n");
        return 1;
    }
    int shm_out = (int32_t)oi.data;
    int shm_err = (int32_t)ei.data;
    int shm_ctl = (int32_t)ci.data;

    /* First attempt with an empty password: root / NOPASSWD succeeds here and
       never shows a dialog. */
    int status = SUDOD_ERR_INTERNAL;
    int shm_in = 0;
    if(sudod_request(target_user.c_str(), prog, args, "",
                     shm_out, shm_err, shm_ctl, &status, &shm_in) != 0) {
        fprintf(stderr, "xsudo: cannot contact sudod (is it running?)\n");
        return 1;
    }

    if(status == SUDOD_OK) {
        int in_w = -1;
        if(vfs_pipe_attach(shm_in, &in_w) != 0) {
            fprintf(stderr, "xsudo: cannot attach stdin\n");
            return 1;
        }
        return relay_and_wait(in_w, out_r, err_r, ctl_r);
    }

    if(status != SUDOD_NEED_PASSWORD) {
        print_sudod_error(status, target_user.c_str(), prog);
        return 1;
    }

    /* Password required: collect it through the widget++ confirmation dialog. */
    X x;
    XSudoWin win(target_user, prog, args, shm_out, shm_err, shm_ctl, commandInfo);
    win.open(&x, 0, 0, 0, 1, 1, "xsudo", XWIN_STYLE_NO_RESIZE, false);
    win.showDialog();

    widgetXRun(&x, &win.dialog);

    if(win.result != XSudoWin::R_ACCEPT) {
        if(win.result == XSudoWin::R_ERROR)
            print_sudod_error(win.ctx.status, target_user.c_str(), prog);
        return 1;
    }

    int in_w = -1;
    if(vfs_pipe_attach(win.ctx.shm_in, &in_w) != 0) {
        fprintf(stderr, "xsudo: cannot attach stdin\n");
        return 1;
    }

    win.close();
    return relay_and_wait(in_w, out_r, err_r, ctl_r);
}
