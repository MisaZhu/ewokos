#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>

#include <sysinfo.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/wait.h>
#include <dirent.h>
#include <sys/shm.h>
#include <ewoksys/usbmsc.h>

#include "cap_policy.h"

#if defined(__has_include)
# if __has_include(<q8b_platform.h>)
#  include <q8b_platform.h>
#  define INIT_HAS_RAMFS_RELEASE 1
# endif
#endif

extern void* read_fs(const char* fname, off_t* size);

/*
 * Load a program from the rootfs.  This runs in the forked child, which has
 * its own address space: SYS_MEM_MAP windows (SD controller / RAM disk) are
 * not inherited across fork, so the backing store must be (re)established in
 * THIS process before read_fs().  That is the BSP's job: bsp_sd_init() both
 * registers the libsd callbacks and maps the windows.  On q8b it re-maps the
 * SDCC2/RAM-disk windows while reusing the card identification the kernel
 * already published, so calling it per child is a cheap remap, not a card
 * re-probe.
 */
static void exec_ready_notify(int fd, uint8_t ready) {
    if(fd >= 0) {
        while(write(fd, &ready, sizeof(ready)) < 0) {
            /* A closed readiness pipe only means the parent stopped waiting. */
            break;
        }
        close(fd);
    }
}

static int32_t exec_from_storage(const char* prog, const char* cmdline,
        int ready_fd) {
    off_t sz;
    char* elf = read_fs(prog, &sz);
    if(elf == NULL) {
        klog("init: read_fs failed (%s)\n", prog);
        exec_ready_notify(ready_fd, 0);
        return -1;
    }

    /* Allocate shm and copy ELF image into it */
    int shm_id = shmget(0, (int)sz, 0666);
    if(shm_id <= 0) {
        klog("init: shmget failed (%s)\n", prog);
        free(elf);
        exec_ready_notify(ready_fd, 0);
        return -1;
    }
    uint8_t* shm_buf = (uint8_t*)shmat(shm_id, NULL, 0);
    if(shm_buf == NULL) {
        klog("init: shmat failed (%s)\n", prog);
        shmctl(shm_id, IPC_RMID, NULL);
        free(elf);
        exec_ready_notify(ready_fd, 0);
        return -1;
    }
    memcpy(shm_buf, elf, sz);
    free(elf);

    /* The ELF is now independent of the source filesystem.  Notify the
     * parent before entering SYS_EXEC_ELF. */
    exec_ready_notify(ready_fd, 1);

    /* Keep the original command line so early daemons receive their options
     * (for example the USB block device and mount point).  The first argument
     * still identifies the ELF image loaded from the filesystem. */
    int res = syscall3(SYS_EXEC_ELF, (ewokos_addr_t)cmdline, (ewokos_addr_t)shm_id, (ewokos_addr_t)sz);
    if(res != 0)
        klog("init: SYS_EXEC_ELF failed %d (%s)\n", res, prog);
    /* If exec fails, clean up shm */
    shmdt(shm_buf);
    shmctl(shm_id, IPC_RMID, NULL);
    return res;
}

#define ROOTFS_SD   0
#define ROOTFS_USB  1
#define ROOTFS_AUTO 2
#define ROOTFS_SELF 3

static int _bootnet;

/* Optional early PCIe owners survive switchroot.  Filter a duplicate entry
 * only when the early profile actually registered the host service. */
static int _pcie_owner_active;

static int write_all(int fd, const char* data, off_t size) {
    off_t done = 0;
    while(done < size) {
        ssize_t n = write(fd, data + done, (size_t)(size - done));
        if(n <= 0)
            return -1;
        done += n;
    }
    return 0;
}

static int make_init_without_pcied(const char* source, const char* target) {
    char text[8192];
    int source_fd = open(source, O_RDONLY);
    if(source_fd < 0) {
        klog("init: filter open failed %s\n", source);
        return -1;
    }
    ssize_t read_size = read(source_fd, text, sizeof(text));
    close(source_fd);
    if(read_size < 0 || read_size >= (ssize_t)sizeof(text)) {
        klog("init: filter read failed %s size=%d\n", source, (int)read_size);
        return -1;
    }
    off_t size = read_size;

    int fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) {
        klog("init: filter target open failed %s\n", target);
        return -1;
    }

    off_t line = 0;
    int rc = 0;
    for(off_t i = 0; i <= size; i++) {
        if(i != size && text[i] != '\n')
            continue;
        off_t end = i < size ? i + 1 : i;
        int skip = 0;
        for(off_t j = line; j < i; j++) {
            if(j + 18 <= i && memcmp(text + j, "/drivers/q8b/pcied", 18) == 0) {
                skip = 1;
                break;
            }
        }
        if(!skip && write_all(fd, text + line, end - line) != 0) {
            rc = -1;
            break;
        }
        line = end;
    }
    close(fd);
    return rc;
}

static const char* filtered_profile_path(const char* source,
        char* path, size_t path_size) {
    const char* name = strrchr(source, '/');
    name = name != NULL ? name + 1 : source;
    snprintf(path, path_size, "/etc/.%s.no-pcied", name);
    return path;
}

/* Create the filtered copies while init is still root.  switch_sys() drops
 * to the system uid before it executes persistent profiles, so doing this
 * lazily from run_init() would fail on a read-only /tmp or root-owned /etc. */
static void prepare_persistent_profiles(void) {
    char source[32];
    char target[64];
    for(int i = 0; i <= 8; i++) {
        if(i == 8)
            snprintf(source, sizeof(source), "/etc/init.rd");
        else
            snprintf(source, sizeof(source), "/etc/init%d.rd", i);
        if(access(source, R_OK) != 0)
            continue;
        filtered_profile_path(source, target, sizeof(target));
        if(make_init_without_pcied(source, target) != 0)
            klog("init: failed to prepare filtered profile %s\n", source);
    }
}

static int run_before_vfs_mode(const char* cmd, int wait_exit) {
    klog("init: %-16s   ", cmd);

    char prog[FS_FULL_NAME_MAX];
    int i = 0;
    while(cmd[i] != 0 && cmd[i] != ' ' && cmd[i] != '\t' &&
            i < FS_FULL_NAME_MAX - 1) {
        prog[i] = cmd[i];
        i++;
    }
    prog[i] = 0;

    int pid = fork();
    if(pid == 0) {
        if(exec_from_storage(prog, cmd, -1) != 0) {
            klog("[failed]!\n");
            exit(-1);
        }
    }
    else if(pid < 0) {
        klog("[failed]!\n");
        return -1;
    }
    else if(wait_exit) {
        ewok_waitpid(pid);
    }
    else if(ipc_wait_ready(pid) != 0) {
        klog("[failed]!\n");
        return -1;
    }
    klog("[ok]\n");
    return 0;
}

static int run_before_vfs(const char* cmd) {
    return run_before_vfs_mode(cmd, 0);
}

static int run_before_vfs_sync(const char* cmd) {
    return run_before_vfs_mode(cmd, 1);
}

/* kernel.conf is part of the boot image, so the rootfs choice is available
 * before the persistent SD filesystem or USB VFS mount exists. */
static int rootfs_policy(void) {
    off_t size = 0;
    char* conf = (char*)read_fs("/etc/kernel/kernel.conf", &size);
    int policy = ROOTFS_AUTO;
    _bootnet = 0;
    if(conf == NULL)
        return policy;
    for(off_t i = 0; i < size; ) {
        off_t end = i;
        while(end < size && conf[end] != '\n')
            end++;
        off_t p = i;
        while(p < end && (conf[p] == ' ' || conf[p] == '\t')) p++;
        if(p + 6 <= end && memcmp(conf + p, "rootfs", 6) == 0) {
            const char* v = conf + p + 6;
            while(v < conf + end && (*v == ' ' || *v == '\t' || *v == '=')) v++;
            if(end - (v - conf) >= 2 && strncmp(v, "sd", 2) == 0) policy = ROOTFS_SD;
            else if(end - (v - conf) >= 3 && strncmp(v, "usb", 3) == 0) policy = ROOTFS_USB;
            else if(end - (v - conf) >= 4 && strncmp(v, "self", 4) == 0) policy = ROOTFS_SELF;
            else if(end - (v - conf) >= 4 && strncmp(v, "auto", 4) == 0) policy = 2;
            /* Continue scanning for optional bootnet=1 recovery setting. */
        }
        if(p + 7 <= end && memcmp(conf + p, "bootnet", 7) == 0) {
            const char* v = conf + p + 7;
            while(v < conf + end && (*v == ' ' || *v == '\t' || *v == '=')) v++;
            if(v < conf + end && *v == '1')
                _bootnet = 1;
        }
        i = end + (end < size ? 1 : 0);
    }
    free(conf);
    return policy;
}

static int start_usb_host(void) {
    return run_before_vfs("/drivers/q8b/usbhostd /dev/usb0 --no-msc-mount");
}

static int start_bootfs(void) {
    /* Boot the BOOT_ONLY initramfs through sdfsd's bootfs mode: bootfs_map()
     * maps the firmware ramdisk directly, so a USB-only boot (no card in the
     * SD slot) can still mount the boot image.  The plain "sdfsd" form probes
     * the physical SD card instead and fails without one, killing the boot
     * before the USB rootfs is ever tried.  The mount also carries the
     * "bootfs:" descriptor switchroot() requires when it moves this
     * filesystem to /boot. */
    return run_before_vfs("/sbin/sdfsd -b /");
}

static int try_usb_rootfs(void) {
    /* The boot USB is an ext3 rootfs.  Reuse the ext3 VFS server used by SD;
     * usbfat32fsd remains available for ordinary FAT32 removable media. */
    return run_before_vfs("/sbin/sdfsd -u /dev/usb0 /");
}

static void enable_usb_automount(void) {
    fsinfo_t info;
    proto_t in;
    if(vfs_get_by_name("/dev/usb0", &info) != 0)
        return;
    PF->init(&in)->addi(&in, 1);
    (void)dev_cntl_by_pid(info.mount_pid, USBMSC_CMD_AUTOMOUNT, &in, NULL);
    PF->clear(&in);
}

#ifdef INIT_HAS_RAMFS_RELEASE
static uint32_t boot_source(void) {
    sys_info_t info;
    if (sys_get_sys_info(&info) != 0)
        return Q8B_BOOT_SOURCE_UNKNOWN;
    return ((const q8b_platform_data_t*)info.platform_data)->rd.boot_source;
}

static const char* boot_source_name(uint32_t source) {
    switch (source) {
    case Q8B_BOOT_SOURCE_SD: return "sd";
    case Q8B_BOOT_SOURCE_USB: return "usb";
    case Q8B_BOOT_SOURCE_NVME: return "nvme";
    case Q8B_BOOT_SOURCE_NET: return "net";
    default: return "unknown";
    }
}

/* NVMe and network rootfs daemons are not part of the Q8B boot image yet.
 * Keep them explicit in the selector so adding those backends later cannot
 * silently change the established SD/USB order. */
static int try_nvme_rootfs(void) {
    klog("init: NVMe rootfs backend unavailable, continuing\n");
    return -1;
}

static int try_net_rootfs(void) {
    klog("init: network rootfs backend unavailable, continuing\n");
    return -1;
}

static int select_rootfs(int policy, int* usb_host_out, int* usb_root_out) {
    uint32_t self = boot_source();
    int usb_host = 0;
    int usb_root = 0;
    int selected = -1;

    klog("init: rootfs policy=%d self=%s order=self,sd,usb,nvme,net\n",
            policy, boot_source_name(self));

    if (policy == ROOTFS_SD) {
        selected = run_before_vfs("/sbin/sdfsd") == 0 ? ROOTFS_SD : -1;
    } else if (policy == ROOTFS_USB) {
        if (start_usb_host() == 0) {
            usb_host = 1;
            if (try_usb_rootfs() == 0) {
                usb_root = 1;
                selected = ROOTFS_USB;
            }
        }
    } else {
        /* Self first, followed by the fixed cross-device order.  Duplicate
         * backends are skipped because the daemon represents the whole class
         * of devices currently visible to the Q8B. */
        uint32_t order[5];
        order[0] = self;
        order[1] = Q8B_BOOT_SOURCE_SD;
        order[2] = Q8B_BOOT_SOURCE_USB;
        order[3] = Q8B_BOOT_SOURCE_NVME;
        order[4] = Q8B_BOOT_SOURCE_NET;
        int tried_sd = 0, tried_usb = 0, tried_nvme = 0, tried_net = 0;
        for (int i = 0; i < 5 && selected < 0; i++) {
            uint32_t source = order[i];
            if (source == Q8B_BOOT_SOURCE_SD) {
                if (tried_sd++)
                    continue;
                if (run_before_vfs("/sbin/sdfsd") == 0)
                    selected = ROOTFS_SD;
            } else if (source == Q8B_BOOT_SOURCE_USB) {
                if (tried_usb++)
                    continue;
                if (!usb_host && start_usb_host() == 0)
                    usb_host = 1;
                if (usb_host && try_usb_rootfs() == 0) {
                    usb_root = 1;
                    selected = ROOTFS_USB;
                }
            } else if (source == Q8B_BOOT_SOURCE_NVME) {
                if (!tried_nvme++)
                    (void)try_nvme_rootfs();
            } else if (source == Q8B_BOOT_SOURCE_NET) {
                if (!tried_net++)
                    (void)try_net_rootfs();
            }
        }
    }
    *usb_host_out = usb_host;
    *usb_root_out = usb_root;
    return selected;
}
#endif

static int run_init(const char* init_file) {
    if(access(init_file, R_OK) != 0) {
        return -1;
    }

    const char* exec_file = init_file;
    char filtered_file[64];
    if(_pcie_owner_active && strcmp(init_file, "/etc/init.ramfs.rd") != 0) {
        filtered_profile_path(init_file, filtered_file, sizeof(filtered_file));
        if(access(filtered_file, R_OK) == 0) {
            exec_file = filtered_file;
            klog("init: filtered persistent pcied entry from %s\n", init_file);
        } else {
            klog("init: filtered profile missing %s\n", filtered_file);
            return -1;
        }
    }

    int pid = fork();
    if(pid == 0) {
        char cmd[FS_FULL_NAME_MAX];
        snprintf(cmd, FS_FULL_NAME_MAX-1, "/bin/shell %s", exec_file);
        //klog("\ninit: loading '%s' ... \n", init_file);
        if(proc_exec(cmd) != 0) {
            //klog("[failed]!\n");
            exit(-1);
        }
    }
    else {
        ewok_waitpid(pid);
    }
    return 0;
}

static void switch_sys(int32_t u_id, int32_t g_id) {
    char initfile[32];
    uint8_t i = 0;

    setuid(u_id);
    setgid(g_id);

    while(i < 8) {
        snprintf(initfile, 31, "/etc/init%d.rd", i);
        if(run_init(initfile) != 0)
            break;
        i++;
    }
    run_init("/etc/init.rd");
}

static void idle(void) {
    while(1);
}

/* The boot RAMFS cannot mount the persistent rootfs (no backend succeeded or
 * switchroot failed): bring up a serial rescue console directly -- the boot
 * image's /etc is read-only, so this must not depend on writing script
 * files.  Init idles afterwards and must not return: it is the first user
 * process. */
static void rescue_shell(void) {
    klog("init: starting rescue shell\n");
    run_before_vfs("/drivers/q8b/uartd /dev/tty0");
    run_before_vfs("/sbin/sessiond");
    run_before_vfs("/bin/bgrun /bin/session -r -t /dev/tty0");
    while(true)
        usleep(100000);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if(((int16_t)getuid()) > 0) {
        klog("process 'init' can only loaded with root!\n");
        return -1;
    }

    if(getpid() != 0) //not first proc, must be cpu core idle
        idle();

    klog("\n[init process started]\n");
    if(run_before_vfs("/sbin/core") != 0 ||
            run_before_vfs("/sbin/vfsd") != 0)
        return -1;

#ifdef INIT_HAS_RAMFS_RELEASE
    /* Mount the boot image at the initial root long enough to execute its
     * early profile.  The same daemon is moved to /boot after the persistent
     * rootfs replaces that initial root. */
    if(start_bootfs() != 0)
        return -1;
    /* Q8B WCN6855 exposes WLAN on PCIe and Bluetooth on USB.  The PCIe
     * owner cold-cycles the shared module (E-key rail, PERST, root-complex
     * reset) during bring-up, and that power-cycle churns the USB bus (the
     * BT function drops and re-enumerates).  When the rootfs itself lives
     * on USB the early usbhostd is already scanning the live bus, so the
     * churn lands on the rootfs device's transfers and transient read
     * failures leak into the boot (observed: btd firmware open() fails with
     * the file present).  Bring the PCIe owner up before the USB host's
     * first root-hub scan for every rootfs source; the persistent
     * profiles' pcied line is filtered once /dev/pcie0 is owned. */
    if(run_before_vfs("/drivers/q8b/pcied /dev/pcie0") == 0)
        klog("init: Q8B PCIe owner ready before USB host\n");
    if(run_init("/etc/init.ramfs.rd") != 0)
        klog("init: boot init profile unavailable\n");
    _pcie_owner_active = dev_get_pid("/dev/pcie0") > 0;
    int policy = rootfs_policy();
    int usb_host = 0;
    int usb_root = 0;
    if(policy == ROOTFS_SELF) {
        /* Maintenance mode: keep the boot RAMFS mounted after the early
         * profile so a newly built pcied can be validated without a second
         * daemon from the persistent SD rootfs resetting the controller. */
        klog("init: rootfs=self; keeping boot RAMFS, switchroot skipped\n");
        rescue_shell();
        while(true)
            usleep(100000);
    } else {
        if(select_rootfs(policy, &usb_host, &usb_root) < 0) {
            klog("init: no rootfs backend succeeded\n");
            rescue_shell();
            return -1;
        }
        klog("init: rootfs selected=%s bootnet=%d\n",
                usb_root ? "usb" : "sd", _bootnet);
    }
    if(policy != ROOTFS_SELF && _bootnet) {
        /* Recovery profile: start the network from the boot image after the
         * SD VFS is available, so SSH remains usable even if an older SD
         * init script cannot reach its normal network stage. */
        (void)run_before_vfs("/drivers/timerd");
        (void)run_before_vfs("/drivers/piped /dev/pipe0");
        (void)run_before_vfs("/drivers/q8b/net /dev/eth0");
        (void)run_before_vfs("/drivers/netd /dev/net0 /dev/eth0");
        /* sshd delegates password validation to sessiond.  Start it from
         * the boot image as well, so the recovery profile remains usable
         * even when the persistent rootfs has an older init script. */
        (void)run_before_vfs("/sbin/sessiond --recovery");
        (void)run_before_vfs("/sbin/sshd");
    }
    if(policy != ROOTFS_SELF) {
        /* Move the boot filesystem to /boot and all device mounts from the
         * RAMFS namespace into the new root's /dev. */
        if(run_before_vfs_sync("/sbin/switchroot") != 0) {
            klog("init: switchroot failed\n");
            rescue_shell();
            return -1;
        }
        /* Q8B's WCN6855 exposes WLAN on PCIe and Bluetooth on USB.  The
         * PCIe owner cold-cycles the shared module during link bring-up.
         * Starting usbhostd first lets its root-hub worker reset/re-enumerate
         * the Bluetooth function while LTSSM is training, which can hold the
         * PCIe endpoint in Detect/Recovery.  For an SD root, finish the PCIe
         * power/link transaction before publishing /dev/usb0.  USB-root
         * booting already needs usbhostd before switchroot and is unchanged.
         */
#if defined(INIT_HAS_RAMFS_RELEASE)
        if (!usb_host && !_pcie_owner_active) {
            if (run_before_vfs("/drivers/q8b/pcied /dev/pcie0") == 0) {
                _pcie_owner_active = 1;
                klog("init: Q8B PCIe owner ready before USB host\n");
            } else {
                klog("init: Q8B PCIe owner failed before USB host\n");
            }
        }
#endif
        /* A USB host used only for automounting starts in the persistent root.
         * A host that supplied the USB rootfs was already moved by switchroot. */
        if(!usb_host)
            (void)start_usb_host();
        if(!usb_root)
            enable_usb_automount();
    }
    if(_pcie_owner_active)
        prepare_persistent_profiles();
#else
    if(run_before_vfs("/sbin/sdfsd") != 0)
        return -1;
#endif

    if(access(CAP_CONF_FILE, R_OK) == 0) { /* no policy file: no cap setup */
        cap_policy_load(CAP_CONF_FILE);
        cap_policy_install(); /* kernel grants the caps at every matching exec */
        switch_sys(1, 1); //run with sys user
    }
    else {
        switch_sys(0, 0); //run with root user
    }

    while(true)
        usleep(100000);
    return 0;
}
