#include <ewoksys/cmain.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/syscall.h>
#include <ewoksys/signal.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/ipc.h>
#include <ewoksys/sys.h>
#include <ewoksys/core.h>
#include <ewoksys/klog.h>
#include <procinfo.h>
#include <unistd.h>
#include <setenv.h>

#ifdef __cplusplus
extern "C" {
#endif

vsyscall_info_t* _vsyscall_info = NULL;
int _current_pid = -1;

static char _cmd[PROC_INFO_MAX_CMD_LEN];
static int _off_cmd;
static const char* _argv0;

static char* read_cmain_arg(void) {
    char* p = NULL;
    uint8_t quotes = 0;

    while(_cmd[_off_cmd] != 0) {
        char c = _cmd[_off_cmd];
        _off_cmd++;
        if(quotes) { //read whole quotes content.
            if(c == '"') {
                _cmd[_off_cmd-1] = 0;
                return p;
            }
            continue;
        }
        if(c == ' ') { //read next arg.
            if(p == NULL) //skip begin spaces.
                continue;
            _cmd[_off_cmd-1] = 0;
            break;
        }
        else if(p == NULL) {
            if(c == '"') { //if start of quotes.
                quotes = 1;
                _off_cmd++;
            }
            p = _cmd + _off_cmd - 1;
        }
    }
    return p;
}

const char* cmain_get_own_dir(char* ret, uint32_t len) {
    static char own_dir[FS_FULL_NAME_MAX];
    if(ret == NULL) {
        ret = own_dir;
        len = FS_FULL_NAME_MAX;
    }
    memset(ret, 0, len);

    int i = strlen(_argv0) - 1;
    while(i >= 0 && i < PROC_INFO_MAX_CMD_LEN && i < len) {
        if(_argv0[i] == '/') {
            strncpy(ret, _argv0, i);
            return ret;
        }
        i--;
    }
    strncpy(ret, "", len);
    return ret;
}

static void close_stdio(void) {
    close(0);
    close(1);
    close(2);
}

static void init_cmd(void) {
    _cmd[0] = 0;
    _off_cmd = 0;
    _argv0 = "";
    syscall3(SYS_PROC_GET_CMD, (ewokos_addr_t)getpid(), (ewokos_addr_t)_cmd, PROC_INFO_MAX_CMD_LEN);
}

#define ARG_MAX 16
extern int __bss_start__;
extern int __bss_end__;

static void loadenv(void) {
    proto_t out;
    PF->init(&out);
    int core_pid = syscall0(SYS_CORE_PID);
    int res = ipc_call(core_pid, CORE_CMD_GET_ENVS, NULL, &out);
    if(res != 0) {
        PF->clear(&out);
        return;
    }
        
    int n = proto_read_int(&out);
    if(n > 0) {
        for(int i=0; i<n; i++) {
            const char* name = proto_read_str(&out);
            const char* value = proto_read_str(&out);
            setenv(name, value);
        }
    }
    PF->clear(&out);
}

static int set_stderr(void) {
    const char* dev = getenv("STDERR_DEV");
    if(dev == NULL || dev[0] == 0)
        return -1;

    int fd = open(dev, O_RDWR);
    if(fd > 0) {
        dup2(fd, 2);
        close(fd);
        return 0;
    }
    return -1;
}

void _libc_init(void);
void _libc_exit(void);

/*
 * Static constructors.
 *
 * GCC collects the address of every namespace-scope object that has a
 * non-trivial initializer into .init_array and expects the C runtime startup
 * code to call them all before main().  EwokOS links applications with
 * -nostartfiles and enters directly at _start() below, so there is no crt0 to
 * do it and, without this loop, those constructors never run at all.
 *
 * Plain C programs mostly do not notice.  C++ ones cannot work: Qt builds its
 * shared-empty container payloads, logging categories and codec tables out of
 * globals initialized this way, and a QApplication whose constructors were
 * skipped dereferences a null refcount almost immediately.
 *
 * The bounds come from PROVIDE_HIDDEN in the linker script, which defines them
 * only because something now references them, and only if an .init_array output
 * section survives into the image.  A program with no static constructors may
 * still link with both left at zero, hence the weak declaration and the test
 * rather than an assumption.  They are compared after decaying to pointers so
 * that -Waddress has nothing to say about taking the address of an array.
 *
 * .fini_array is deliberately not run.  By the time the process is on its way
 * out, proc_exit() has already torn down the IPC and vfs state a destructor
 * would need in order to flush anything, so calling them would trade a clean
 * exit for a crash.  Nothing in the tree registers a .fini_array entry that
 * has to run.
 */
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

static void run_init_array(void) {
    void (**p)(void) = __init_array_start;
    void (**end)(void) = __init_array_end;

    if(p == NULL || end == NULL)
        return;

    while(p < end) {
        if(*p != NULL)
            (*p)();
        p++;
    }
}

void _start(void) {
    char* argv[ARG_MAX] = {0};
    int32_t argc = 0;
    //clean bss befor cmain
    int *p = &__bss_start__;
    while(p < &__bss_end__){
        *p++ = 0;
    }

    _current_pid = -1;
    _current_pid = getpid();

    _vsyscall_info = (vsyscall_info_t*)syscall0(SYS_GET_VSYSCALL_INFO);
    if(_vsyscall_info == NULL)
        exit(-1);

    _libc_init();
    //__ewok_malloc_init();
    proc_init();
    sys_signal_init();
    vfs_init();
    init_cmd();

    while(argc < ARG_MAX) {
        char* arg = read_cmain_arg(); 
        if(arg == NULL || arg[0] == 0)
            break;
        if(argc == 0)
            _argv0 = arg;
        argv[argc] = (char*)malloc(strlen(arg)+1);
        if(argv[argc] != NULL)
            strcpy(argv[argc], arg);
        argc++;
    }

    // int val = setenv("PATH", "/bin", 1);
    // klog("setenv: %d\n", val);
    // // const char* paths = getenv("PATH");
    // // klog("PATH: %s\n", paths);

    loadenv();
    set_stderr();

    /* Last thing before main(): every service a constructor might need - the
       heap, the vfs, signals, the command line and the environment - is up by
       now, and running them any earlier would leave a constructor that calls
       malloc or getenv working against uninitialized state. */
    run_init_array();

    int ret = main(argc, argv);
    /*
     * Let the process-exit path reclaim inherited stdio FDs in one place.
     * Closing 0/1/2 explicitly here is redundant and can disturb shared
     * socket-backed consoles (for example telnet shell children after fork).
     */
    //__ewok_malloc_close();
    proc_exit();

    argc = 0;
    while(argc < ARG_MAX) {
        if(argv[argc] != NULL)
            free(argv[argc]);
        argc++;
    }
    exit(ret);
}

#ifdef __cplusplus
}
#endif
