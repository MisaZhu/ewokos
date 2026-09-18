# 11 The Boot Process: init and the Shell

> Language: **English** | [中文](11-init.zh.md)
>
> Goal: starting from the moment the kernel loads the first process, fully
> understand how userland is built up layer by layer; understand the `.rd`
> boot scripts — all the way until a command-line prompt appears in front of
> you.

## 11.1 The First User Process: /sbin/init

Recall Ch. 10: at the end of kernel startup, `load_init_proc()` is called
to read `/sbin/init`'s ELF straight off the SD card and create a process.
This process has a few special traits:

- **pid = 0** (EwokOS's process numbering starts from 0);
- **uid = -1** (a special mark: loaded by the kernel, holding the highest
  trust);
- It is the ancestor of all processes, responsible for adopting orphans
  (Ch. 07).

Look at `main` in [init.c](../../system/basic/sys/init/init.c):

```c
int main(int argc, char** argv) {
    if(((int16_t)getuid()) >= 0) {   // not loaded by the kernel? refuse to run
        klog("process 'init' can only loaded by kernel!\n");
        return -1;
    }
    if(getpid() != 0)                // not pid 0? then you're a core's idle process
        idle();

    syscall1(SYS_PROC_SET_CMD, (ewokos_addr_t)"/sbin/init");

    klog("\n[init process started]\n");
    run_before_vfs("/sbin/core");    // ① name service + system coordination
    run_before_vfs("/sbin/vfsd");    // ② the virtual filesystem
    run_before_vfs("/sbin/sdfsd");   // ③ the SD card ext2 filesystem

    switch_root();                   // ④ run the boot scripts
    while(true)
        usleep(100000);              // init never exits
}
```

Note the timing of ①②③: at this point **the VFS doesn't exist yet** and no
filesystem is mounted! So these three processes cannot be started by
"loading from a file" — they use `run_before_vfs`:

```c
static void run_before_vfs(const char* cmd) {
    int pid = fork();
    if(pid == 0) {
        exec_from_sd(cmd);   // use the kernel-mode SD driver to read the ELF
                             // from ext2 directly, then load it via SYS_EXEC_ELF
    }
    else
        ipc_wait_ready(pid); // the parent waits until the service is truly ready
}
```

This is the classic "chicken and egg" solution: the filesystem services
themselves must be started by unconventional means first.

## 11.2 The Boot Scripts: /etc/init.rd

With the three core services in place, the filesystem is usable, and `init`
moves on to the normal flow — executing `/etc/init0.rd` ~ `/etc/init7.rd`
in order (only those that exist), and finally `/etc/init.rd`:

```c
static void switch_root(void) {
    char initfile[32];
    uint8_t i = 0;
    while(i < 8) {
        snprintf(initfile, 31, "/etc/init%d.rd", i);
        if(run_init(initfile) != 0)
            break;
        i++;
    }
    run_init("/etc/init.rd");
}
// inside run_init:  fork() + proc_exec("/bin/shell <initfile>")
```

In other words: **the boot scripts are interpreted and executed by the
shell**.

### `.rd` Script Syntax

Take the Pi's basic boot script
[etc/basic/init.rd](../../machines/raspix/system/etc/basic/init.rd) as an
example:

```sh
@/bin/ipcserv /drivers/raspix/uartd  /dev/tty0   # UART driver (the terminal)
@/bin/ipcserv /drivers/raspix/cpud   /dev/cpu    # CPU info driver
@set_stdio /dev/tty0                              # hook stdio up to the UART

@/bin/ipcserv /drivers/timerd                    # the timer device
@/bin/ipcserv /drivers/piped      /dev/pipe0     # pipes
@/bin/ipcserv /drivers/ramfsd     /tmp           # the RAM filesystem
@/bin/ipcserv /drivers/nulld      /dev/null

@/bin/ipcserv /sbin/sessiond                     # session management
@/bin/bgrun /bin/session -r -t /dev/tty0         # start a terminal session → the prompt appears!
```

Lines starting with `@` are special shell directives:

| Directive | Effect |
|------|------|
| `@/bin/ipcserv <program> [mount point]` | start a program as a background service (can register with core) |
| `@/bin/bgrun <command>` | run in the background |
| `@set_stdio <device>` | switch the standard input/output device |
| `@export VAR=value` | set an environment variable (e.g. `TZ` timezone, `XTHEME` theme) |
| `@echo ...` | print |

The full (graphical) system's script is at
[etc/xwin/init.rd](../../machines/raspix/system/etc/xwin/init.rd); it
starts the display driver, fonts, the splash screen, the X server, and so
on in sequence. Every `@/bin/splash -m "..." -p xx` line advances the boot
progress bar once.

> Summary of the configuration file system: the kernel config is
> `/etc/kernel/kernel.conf` (simple key-value pairs); each service uses
> JSON config (e.g. `/etc/console.json`); boot orchestration uses `.rd`
> scripts. All three follow "whoever uses it reads it" — there is no
> central configuration manager.

## 11.3 Sessions and the Shell: How the Prompt Appears

The boot script's last line, `/bin/session -r -t /dev/tty0`, starts a
terminal session:

```
session ──► opens /dev/tty0 (the serial device file, backed by the uartd process)
    │
    └──► fork + exec /bin/shell
              │
              └──► infinite loop: read a line → parse → fork+exec the command
                   → wait → print the prompt
```

`shell` ([system/basic/bin/shell/](../../system/basic/bin/shell/)) is the
program you interact with behind the prompt. What it does is familiar to
every Unix veteran:

1. Read a line from standard input;
2. Parse the command and arguments (supporting `|` pipes, `>` redirection,
   `&` background);
3. `fork()` a child, `exec()` the target program;
4. Without `&`, `waitpid` for the child to finish;
5. Print the next prompt.

Pipes are implemented with `/dev/pipe0` (the piped process), and
redirection is just `open`ing a file and replacing standard input/output —
all built on the mechanisms of chapters 09 and 10.

## 11.4 The Complete Boot Sequence Diagram

Stringing this chapter together with the previous ones:

```
kernel startup complete
  │
  ├─ load_init_proc(): read /sbin/init off the SD card, create process 0
  ▼
init (pid 0):
  ├─ exec_from_sd /sbin/core      (the name service)
  ├─ exec_from_sd /sbin/vfsd      (the VFS mount table is set up)
  ├─ exec_from_sd /sbin/sdfsd     (mounts /, the SD card filesystem)
  ├─ the shell executes /etc/init0.rd ... /etc/init.rd
  │     ├─ uartd      → /dev/tty0
  │     ├─ timerd     → /dev/timer
  │     ├─ ramfsd     → /tmp
  │     ├─ logd       → /dev/log
  │     ├─ (graphical targets also have) displayd / fontd / xserverd ...
  │     └─ session → shell
  ▼
the prompt appears, waiting for your command
```

## 11.5 Follow Along: Reshape Your Boot Script

A very satisfying experiment — modifying the boot flow:

```bash
> cd machines/raspix/system/etc/basic
> edit init.rd, adding a line:
  @echo Hello from my init script!
> cd machines/raspix/system
> make && make sd          # rebuild and repack the image
> make run
```

You'll see your own words appear in the boot log. That is the entire
process of "configuring the system's boot behavior" — not a single line of
C code needed.

## 11.6 Exercises

1. After booting, run `ps` and check every process against the sequence
   diagram in §11.4, in boot order;
2. Modify `init.rd`: comment out `timerd` and observe what happens to the
   system (hint: `sleep` and `ps`'s time statistics will be affected);
3. Read the [shell source directory](../../system/basic/bin/shell/) and
   find the code segment that "parses the pipe character `|`".

## 11.7 Summary

- `init` (pid 0) is the only "loaded directly by the kernel" process;
  afterwards it is responsible for bringing up everything;
- Before the VFS is ready, the core services are loaded straight off the SD
  card with `exec_from_sd`;
- Boot orchestration is done by the `/etc/init*.rd` scripts, interpreted by
  the shell, with `@`-prefixed directives;
- `session + shell` turns the serial port into the command line in front of
  you.

Next chapter: no longer just booting other people's programs — write a
command of your own.
