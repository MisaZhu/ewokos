#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/errno.h>
#include <ewoksys/mstr.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <ewoksys/proc.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <ewoksys/core.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/ipc_serv.h>
#include <sysinfo.h>
#include <kevent.h>
#include <procinfo.h>
#include <ewoksys/hashmap.h>

typedef struct {
    str_t* cwd;	
    map_t envs;
} proc_info_t;

static proc_info_t *_proc_info_table = NULL;
static uint32_t _max_proc_table_num = 0;

static void core_init(void) {
    int32_t i;

    sys_info_t sysinfo;
    sys_get_sys_info(&sysinfo);
    _max_proc_table_num = sysinfo.max_task_num;
    _proc_info_table = (proc_info_t*)malloc(_max_proc_table_num*sizeof(proc_info_t));

    for(i = 0; i<_max_proc_table_num; i++) {
        _proc_info_table[i].cwd = str_new("/");
        _proc_info_table[i].envs = hashmap_new(16);
    }
}

static map_t _ipc_servs = NULL; //pids of ipc_servers
static int get_ipc_serv(const char* key) {
    int32_t *v;
    if(hashmap_get(_ipc_servs, key, (void**)&v) == MAP_MISSING) {
        return -1;
    }
    return *v;
}

static void do_ipc_serv_get(proto_t* in, proto_t* out) {
    const char* ks_id = proto_read_str(in);
    if(ks_id[0] == 0) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, get_ipc_serv(ks_id));
}

static void do_ipc_serv_reg(int pid, proto_t* in, proto_t* out) {
    const char* ks_id = proto_read_str(in);
    if(ks_id[0] == 0) {
        PF->addi(out, -1);
        return;
    }

    int32_t* v = (int32_t*)malloc(sizeof(int32_t));
    *v = pid;
    if(hashmap_put(_ipc_servs, ks_id, v) != MAP_OK) {
        PF->addi(out, -1);
        return;
    }
    PF->addi(out, 0);
}

static void do_ipc_serv_unreg(int pid, proto_t* in, proto_t* out) {
    const char* ks_id = proto_read_str(in);
    if(ks_id[0] == 0) {
        PF->addi(out, -1);
        return;
    }

    int32_t *v;
    if(hashmap_get(_ipc_servs, ks_id, (void**)&v) == MAP_MISSING) {
        PF->addi(out, -1);
        return;
    }

    if(*v != pid) {
        PF->addi(out, -1);
        return;
    }

    hashmap_remove(_ipc_servs, ks_id);
    free(v);
    PF->addi(out, 0);
}

static void do_proc_get_cwd(int pid, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || pid >= _max_proc_table_num)
        return;
    PF->clear(out)->addi(out, 0)->adds(out, CS(_proc_info_table[pid].cwd));
}

static int get_fsinfo_by_name(const char* fname, fsinfo_t* info) {
    proto_t in, out;
    PF->init(&in)->adds(&in, fname);
    PF->init(&out);
    int pid = get_ipc_serv(IPC_SERV_VFS);
    int res = ipc_call(pid, VFS_GET_BY_NAME, &in, &out);
    PF->clear(&in);
    if(res == 0) {
        res = proto_read_int(&out); //res = node
        if(res != 0) {
            if(info != NULL)
                proto_read_to(&out, info, sizeof(fsinfo_t));
            res = 0;
        }
        else
            res = -1;
    }
    PF->clear(&out);
    return res;	
}

static void do_proc_set_cwd(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || pid >= _max_proc_table_num)
        return;

    const char* s = proto_read_str(in);
    fsinfo_t info;
    if(get_fsinfo_by_name(s, &info) != 0) {
        PF->addi(out, ENOENT);
        return;
    }
    
    if(vfs_check_access(pid, &info, X_OK) != 0) {
        PF->addi(out, EPERM);
        return;
    }

    str_cpy(_proc_info_table[pid].cwd, s);
    PF->clear(out)->addi(out, 0);
}

static str_t* env_get(map_t envs, const char* key) {
    if(envs == NULL || key == NULL)
        return NULL;
    str_t* ret = NULL;
    if(hashmap_get(envs, key, (void**)&ret) == MAP_OK) {
        return ret;
    }
    return NULL;
}

static void set_env(map_t envs, const char* key, const char* val) {
    if(envs == NULL || key == NULL || val == NULL)
        return;
    str_t* v = env_get(envs, key);
    if(v != NULL) {
        str_cpy(v, val);
    }
    else {
        v = str_new(val);
        if(v != NULL) {
            hashmap_put(envs, key, v);
        }
    }
}

static void do_proc_set_env(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || pid >= _max_proc_table_num)
        return;
    if(_proc_info_table[pid].envs == NULL)
        return;
    const char* key = proto_read_str(in);
    const char* val = proto_read_str(in);

    set_env(_proc_info_table[pid].envs, key, val);
    PF->clear(out)->addi(out, 0);
}

static int get_envs(map_t map, const char* key, any_t data, any_t arg) {
    (void)map;
    proto_t* out = (proto_t*)arg;
    str_t* v = (str_t*)data;

    if(key == NULL || v == NULL || out == NULL)
        return MAP_OK;
    PF->adds(out, key)->adds(out, v->cstr);
    return MAP_OK;
}

static void do_proc_get_envs(int pid, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || pid >= _max_proc_table_num)
        return;
    if(_proc_info_table[pid].envs == NULL) {
        PF->clear(out)->addi(out, 0);
        return;
    }

    PF->clear(out)->addi(out, hashmap_length(_proc_info_table[pid].envs));
    hashmap_iterate(_proc_info_table[pid].envs, get_envs, out);
}

static void do_proc_get_env(int pid, proto_t* in, proto_t* out) {
    PF->addi(out, -1);
    if(pid < 0 || pid >= _max_proc_table_num)
        return;
    if(_proc_info_table[pid].envs == NULL)
        return;
    const char* key = proto_read_str(in);
    str_t* v = env_get(_proc_info_table[pid].envs, key);
    if(v == NULL) {
        return;
    }
    PF->clear(out)->addi(out, 0)->adds(out, v->cstr);
}

static int copy_envs(map_t map, const char* key, any_t data, any_t arg) {
    (void)map;
    map_t to = (map_t)arg;
    str_t* v = (str_t*)data;
    if(to == NULL || key == NULL || v == NULL)
        return MAP_OK;
    set_env(to, key, v->cstr);
    return MAP_OK;
}

static int free_envs(map_t map, const char* key, any_t data, any_t arg) {
    (void)map;
    (void)key;
    str_t* v = (str_t*)data;
    if(v != NULL) {
        str_free(v);
    }
    return MAP_OK;
}

static void do_proc_clone(int fpid, int cpid) {
    if(fpid < 0 || fpid >= _max_proc_table_num ||
            cpid < 0 || cpid >= _max_proc_table_num)
        return;

    if(_proc_info_table[cpid].cwd != NULL && _proc_info_table[fpid].cwd != NULL) {
        str_cpy(_proc_info_table[cpid].cwd, CS(_proc_info_table[fpid].cwd));
    }

    if(_proc_info_table[cpid].envs != NULL) {
        hashmap_iterate(_proc_info_table[cpid].envs, free_envs, NULL);
        hashmap_free(_proc_info_table[cpid].envs);
    }
    _proc_info_table[cpid].envs = hashmap_new(16);

    if(_proc_info_table[fpid].envs != NULL && _proc_info_table[cpid].envs != NULL) {
        hashmap_iterate(_proc_info_table[fpid].envs, copy_envs, _proc_info_table[cpid].envs);
    }
}

static void handle_ipc(int pid, int cmd, proto_t* in, proto_t* out, void* p) {
    (void)p;
    pid = proc_getpid(pid);

    switch(cmd) {
    case CORE_CMD_IPC_SERV_REG: //regiester ipc_server pid
        do_ipc_serv_reg(pid, in, out);
        return;
    case CORE_CMD_IPC_SERV_UNREG: //unregiester ipc_server pid
        do_ipc_serv_unreg(pid, in, out);
        return;
    case CORE_CMD_IPC_SERV_GET: //get ipc_server pid
        do_ipc_serv_get(in, out);
        return;
    case CORE_CMD_SET_CWD:
        do_proc_set_cwd(pid, in, out);
        return;
    case CORE_CMD_GET_CWD:
        do_proc_get_cwd(pid, out);
        return;
    case CORE_CMD_SET_ENV:
        do_proc_set_env(pid, in, out);
        return;
    case CORE_CMD_GET_ENV:
        do_proc_get_env(pid, in, out);
        return;
    case CORE_CMD_GET_ENVS:
        do_proc_get_envs(pid, out);
        return;
    }
}

/*----kernel event -------*/

static void do_proc_created(kevent_t* kev) {
    int fpid = kev->data[0];
    int cpid = kev->data[1];

    int pid = get_ipc_serv(IPC_SERV_VFS);
    if(pid > 0) {
        proto_t data;
        PF->init(&data)->addi(&data, fpid)->addi(&data, cpid);
        ipc_enable();
        ipc_call_wait(pid, VFS_PROC_CLONE, &data);
        ipc_disable();
        PF->clear(&data);
    }
    do_proc_clone(fpid, cpid);
    proc_wakeup(cpid);
    proc_wakeup(fpid);
}

static void do_proc_exit(kevent_t* kev) {
    int pid = kev->data[0];
    proto_t data;
    PF->init(&data)->addi(&data, pid);
    int vfs_pid = get_ipc_serv(IPC_SERV_VFS);
    if(vfs_pid > 0) {
        ipc_enable();
        ipc_call_wait(vfs_pid, VFS_PROC_EXIT, &data);
        ipc_disable();
    }

    if(pid >= 0 && pid < _max_proc_table_num && _proc_info_table[pid].envs != NULL) {
        hashmap_iterate(_proc_info_table[pid].envs, free_envs, NULL);
        hashmap_free(_proc_info_table[pid].envs);
        _proc_info_table[pid].envs = hashmap_new(16);
    }

    PF->clear(&data);
}

static const char* core_dump_reason_str(uint32_t reason) {
    switch(reason) {
    case KEV_CORE_DUMP_UNDEF:    return "undef-instruction";
    case KEV_CORE_DUMP_PREFETCH: return "prefetch-abort";
    case KEV_CORE_DUMP_DATA:     return "data-abort";
    default:                     return "unknown";
    }
}

/* bounded vsnprintf appender: returns the new write offset, clamped to cap-1 */
static int dump_appendf(char* buf, int cap, int off, const char* fmt, ...) {
    if(off < 0 || off >= cap-1)
        return off < 0 ? 0 : off;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf+off, cap-off, fmt, ap);
    va_end(ap);
    if(n < 0)
        return off;
    off += n;
    if(off > cap-1)
        off = cap-1;
    return off;
}

/*
 * A user proc died from an exception. The kernel already tore the proc down, so
 * its procinfo may be gone; textualize the crash snapshot (header plus the full
 * arch register file, mirroring the kernel's dump_ctx) and hand it straight to
 * logd (CTRL_WRITE) instead of opening /dev/log.
 */
static void do_proc_core_dump(kevent_t* kev) {
    kev_core_dump_t* d = &kev->core_dump;

    int logd_pid = get_ipc_serv(IPC_SERV_LOG);
    if(logd_pid <= 0)
        return;

    char buf[2048];
    int cap = (int)sizeof(buf);
    int off = dump_appendf(buf, cap, 0,
            "[coredump] pid=%d core=%u reason=%s status=0x%x fault=0x%llx pc=0x%llx sp=0x%llx\n",
            d->pid, d->core, core_dump_reason_str(d->reason), d->status,
            (unsigned long long)d->fault_addr,
            (unsigned long long)d->pc,
            (unsigned long long)d->sp);

#if defined(__aarch64__)
    off = dump_appendf(buf, cap, off, "pc=0x%llx spsr=0x%llx sp=0x%llx lr=0x%llx\n",
            (unsigned long long)d->regs.aarch64.pc,
            (unsigned long long)d->regs.aarch64.spsr_el1,
            (unsigned long long)d->regs.aarch64.sp,
            (unsigned long long)d->regs.aarch64.lr);
    for(int i=0; i<30; i++) {
        if(i > 0 && i%4 == 0)
            off = dump_appendf(buf, cap, off, "\n");
        off = dump_appendf(buf, cap, off, "x%02d=0x%llx ", i,
                (unsigned long long)d->regs.aarch64.gpr[i]);
    }
    off = dump_appendf(buf, cap, off, "\n");
#elif defined(__arm__)
    off = dump_appendf(buf, cap, off, "cpsr=0x%x pc=0x%x sp=0x%x lr=0x%x\n",
            (unsigned int)d->regs.arm.cpsr,
            (unsigned int)d->regs.arm.pc,
            (unsigned int)d->regs.arm.sp,
            (unsigned int)d->regs.arm.lr);
    for(int i=0; i<13; i++) {
        if(i > 0 && i%4 == 0)
            off = dump_appendf(buf, cap, off, "\n");
        off = dump_appendf(buf, cap, off, "r%d=0x%x ", i,
                (unsigned int)d->regs.arm.gpr[i]);
    }
    off = dump_appendf(buf, cap, off, "\n");
#elif defined(__x86_64__) || defined(__i386__)
    off = dump_appendf(buf, cap, off, "cr2=0x%llx trap=0x%llx err=0x%llx\n",
            (unsigned long long)d->regs.x86.cr2,
            (unsigned long long)d->regs.x86.trap_no,
            (unsigned long long)d->regs.x86.err_code);
    off = dump_appendf(buf, cap, off, "pc=0x%llx lr=0x%llx sp=0x%llx cs=0x%llx ss=0x%llx flags=0x%llx\n",
            (unsigned long long)d->regs.x86.pc,
            (unsigned long long)d->regs.x86.lr,
            (unsigned long long)d->regs.x86.sp,
            (unsigned long long)d->regs.x86.cs,
            (unsigned long long)d->regs.x86.ss,
            (unsigned long long)d->regs.x86.rflags);
    {
        static const char* xnames[15] = {
            "rdi","rsi","rdx","rcx","r8","r9","rax","rbx",
            "rbp","r10","r11","r12","r13","r14","r15"
        };
        for(int i=0; i<15; i++) {
            if(i > 0 && i%4 == 0)
                off = dump_appendf(buf, cap, off, "\n");
            off = dump_appendf(buf, cap, off, "%s=0x%llx ", xnames[i],
                    (unsigned long long)d->regs.x86.gpr[i]);
        }
    }
    off = dump_appendf(buf, cap, off, "\n");
#elif defined(__riscv)
    off = dump_appendf(buf, cap, off, "pc=0x%lx ra=0x%lx sp=0x%lx gp=0x%lx tp=0x%lx\n",
            d->regs.riscv.pc, d->regs.riscv.ra, d->regs.riscv.sp,
            d->regs.riscv.gp, d->regs.riscv.tp);
    off = dump_appendf(buf, cap, off, "t0=0x%lx t1=0x%lx t2=0x%lx s0=0x%lx s1=0x%lx\n",
            d->regs.riscv.t0, d->regs.riscv.t1, d->regs.riscv.t2,
            d->regs.riscv.s0, d->regs.riscv.s1);
    for(int i=0; i<8; i++)
        off = dump_appendf(buf, cap, off, "a%d=0x%lx ", i, d->regs.riscv.gpr[i]);
    off = dump_appendf(buf, cap, off, "\n");
    off = dump_appendf(buf, cap, off,
            "s2=0x%lx s3=0x%lx s4=0x%lx s5=0x%lx s6=0x%lx s7=0x%lx\n",
            d->regs.riscv.s2, d->regs.riscv.s3, d->regs.riscv.s4,
            d->regs.riscv.s5, d->regs.riscv.s6, d->regs.riscv.s7);
    off = dump_appendf(buf, cap, off,
            "s8=0x%lx s9=0x%lx s10=0x%lx s11=0x%lx t3=0x%lx t4=0x%lx t5=0x%lx t6=0x%lx\n",
            d->regs.riscv.s8, d->regs.riscv.s9, d->regs.riscv.s10, d->regs.riscv.s11,
            d->regs.riscv.t3, d->regs.riscv.t4, d->regs.riscv.t5, d->regs.riscv.t6);
    off = dump_appendf(buf, cap, off, "sstatus=0x%lx sbadaddr=0x%lx scause=0x%lx\n",
            d->regs.riscv.sstatus, d->regs.riscv.sbadaddr, d->regs.riscv.scause);
#endif
    if(off <= 0)
        return;

    proto_t in;
    PF->init(&in)->adds(&in, buf);
    ipc_enable();
    dev_cntl_by_pid(logd_pid, CTRL_WRITE, &in, NULL);
    ipc_disable();
    PF->clear(&in);
}

static void handle_event(kevent_t* kev) {
    switch(kev->type) {
    case KEV_PROC_EXIT:
        do_proc_exit(kev);
        return;
    case KEV_PROC_CREATED:
        do_proc_created(kev);
        return;
    case KEV_PROC_CORE_DUMP:
        do_proc_core_dump(kev);
        return;
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    core_init();
    _ipc_servs = hashmap_new(16);

    ipc_serv_run(handle_ipc, NULL, NULL, IPC_NON_BLOCK);
    syscall0(SYS_CORE_READY);

    while(1) {
        kevent_t kev;
        if(syscall1(SYS_GET_KEVENT, (ewokos_addr_t)&kev) == 0) {
            ipc_disable();
            handle_event(&kev);
            ipc_enable();
        }
        else
            proc_usleep(50000);
    }

    hashmap_free(_ipc_servs);
    free(_proc_info_table);
    return 0;
}
