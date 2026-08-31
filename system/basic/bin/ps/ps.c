#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>
#include <string.h>
#include <ewoksys/session.h>

static const char* _states[] = {
    "unu", //0
    "crt", //1
    "slp", //2
    "wat", //3
    "blk", //4
    "rdy", //5
    "run", //6
    "zmb"  //7
};

static const char* get_state(procinfo_t* proc) {
    static char ret[16];
    if(proc->state == 4) {
        strcpy(ret, "blk");
    }
    else if(proc->state == 3)
        snprintf(ret, 13, "wat[%d]", proc->wait_for);
    else {
        strcpy(ret, _states[proc->state]);
        if((proc->state == 5 || proc->state == 6) && proc->priority > 0) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%s(%d)", ret, proc->priority);
            strcpy(ret, tmp);
        }
    }
    return ret;
}

static const char* get_owner(procinfo_t* proc) {
    if(proc->uid < 0)
        return "kernel";

    session_info_t info;
    static char name[SESSION_USER_MAX+1] = {0};

    if(session_get_by_uid(proc->uid, &info) == 0)
        strncpy(name, info.user, SESSION_USER_MAX);
    else
        snprintf(name, SESSION_USER_MAX, "%d", proc->uid);

    return name;
}

static const char* get_cmd(procinfo_t* proc, int full) {
    if(!full) {
        char* p = proc->cmd;
        while(*p != 0) {
            if(*p == ' ') {
                *p = 0;
                break;
            }
            p++;
        }
    }	
    return proc->cmd;
}

static const char* get_core_loading(sys_info_t* sys_info, procinfo_t* proc, uint32_t run_usec) {
    static char ret[16];
    if(sys_info->cores > 1)
        snprintf(ret, sizeof(ret), "%d:%d%%", proc->core, run_usec/10000);
    else
        snprintf(ret, sizeof(ret), "%d%%", run_usec/10000);
    return ret;
}

static uint32_t get_loading(procinfo_t* procs, int num, procinfo_t* proc, int8_t thread) {
    uint32_t run_usec = proc->run_usec;
    if(thread == 0 && proc->type == TASK_TYPE_PROC) { //aggregate all thread loadings
        for(int i=0; i<num; i++) {
            procinfo_t* p = &procs[i];
            if(p->type == TASK_TYPE_THREAD && p->father_pid == proc->pid)
                run_usec += p->run_usec;
        }
    }
    return run_usec;
}

static int get_thread_num(procinfo_t* procs, int num, procinfo_t* proc) {
    int n = 0;
    for(int i=0; i<num; i++) {
        procinfo_t* p = &procs[i];
        if(p->type == TASK_TYPE_THREAD && p->father_pid == proc->pid)
            n++;
    }
    return n;
}

static int doargs(int argc, char* argv[], int8_t* full, int8_t* all, int8_t* thread) {
    *full = 0;
    *all = 0;
    *thread = 0;

    int c = 0;
  while (c != -1) {
    c = getopt (argc, argv, "aft");
    if(c == -1)
      break;

    switch (c) {
      case 't':
        *thread = 1;
        break;
      case 'a':
        *all = 1;
        break;
      case 'f':
        *full = 1;
        break;
      case '?':
        return -1;
      default:
        c = -1;
        break;
    }
  }
    return 0;
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int8_t all;
    int8_t thread;
    int8_t full;
    if(doargs(argc, argv, &full, &all, &thread) != 0)
        return -1;

    procinfo_t cprocinfo;
    if(proc_info(getpid(), &cprocinfo) != 0)
        return -1;
    
    int uid = getuid();

    int num = 0;
    sys_info_t sys_info;
    sys_state_t sys_state;
    sys_get_sys_info(&sys_info);
    syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)&sys_state);
    uint32_t fr_mem = sys_state.mem.free / (1024*1024);
    uint32_t shm_mem = sys_state.mem.shared / (1024*1024);
    uint32_t t_mem = sys_info.total_usable_mem_size / (1024*1024);
    uint32_t csec = (uint32_t)(sys_state.kernel_usec / 1000000);

    num = syscall0(SYS_GET_PROCS_NUM);

    procinfo_t* procs = (procinfo_t*)malloc(sizeof(procinfo_t)*num);
    int got = -1;
    if(num > 0 && procs != NULL)
        got = syscall2(SYS_GET_PROCS, (ewokos_addr_t)num, (ewokos_addr_t)procs);
    if(got >= 0) {
        num = got;
        if(full)
            printf("OWNER    PID  FATH  CORE  STATE     TIME     HEAP    SHM    PROC\n"); 
        else
            printf("OWNER    PID  FATH  CORE  STATE     PROC\n"); 
        for(int i=0; i<num; i++) {
            procinfo_t* proc = &procs[i];
            if(uid > 0 && proc->uid != cprocinfo.uid && all == 0) //for current uid
                continue;

            if(proc->type != TASK_TYPE_PROC && thread == 0) //for thread 
                continue;

            if(full) {
                uint32_t sec = csec - proc->start_sec;
                char heap_size[32] = {0};
                char shm_size[32] = {0};
                printf("%-8s %-4d %-4d  %-5s %-9s %02d:%02d:%02d %-6s  %-5s  %s",
                    get_owner(proc),
                    proc->pid,
                    proc->father_pid,
                    get_core_loading(&sys_info, proc, get_loading(procs, num, proc, thread)),
                    get_state(proc),
                    sec / (3600),
                    sec / 60,
                    sec % 60,
                    get_mem_size_desc(proc->heap_size, heap_size),
                    get_mem_size_desc(proc->shm_size, shm_size),
                    get_cmd(proc, full));
            }
            else {
                printf("%-8s %-4d %-4d  %-5s %-9s %s",
                    get_owner(proc),
                    proc->pid,
                    proc->father_pid,
                    get_core_loading(&sys_info, proc, get_loading(procs, num, proc, thread)),
                    get_state(proc),
                    get_cmd(proc, full));

            }

            if(proc->type == TASK_TYPE_THREAD)
                printf(" [THRD:%d]\n", proc->father_pid);
            else {
                int tnum = (thread == 0) ? get_thread_num(procs, num, proc) : 0;
                if(tnum > 0)
                    printf(" [%dt]\n", tnum);
                else
                    printf("\n");
            }
        }
    }
    printf("\n\033[1mtask_num: %d, memory: total %d MB, free %d MB, shm %d MB\n", num, t_mem, fr_mem, shm_mem);
    printf("cpu idle:");
    for(uint32_t i=0; i<sys_info.cores; i++) {
        int idle = sys_info.core_idles[i]/10000;
        printf("  %d%%", idle > 100 ? 100 : idle);
    }
    printf("\033[0m\n");

    if(procs != NULL)
        free(procs);
    return 0;
}
