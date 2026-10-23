#include <procinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>

/*
 * Extract the program name out of a procinfo cmd string.
 *
 * cmd may hold a full command line ("/bin/foo arg1 arg2"), so take the first
 * whitespace separated token and then strip any leading directory part, leaving
 * just "foo". The result is written into the caller supplied buffer.
 */
static void get_proc_name(const procinfo_t* proc, char* name, int size) {
    const char* p = proc->cmd;
    const char* start = p;
    const char* end = p;

    while(*end != 0 && *end != ' ' && *end != '\t')
        end++;

    /* keep only the basename of the first token */
    for(const char* q = start; q < end; q++) {
        if(*q == '/')
            start = q + 1;
    }

    int len = (int)(end - start);
    if(len >= size)
        len = size - 1;
    if(len < 0)
        len = 0;

    memcpy(name, start, len);
    name[len] = 0;
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int sig = SYS_SIG_KILL;

    if(argc < 2) {
        printf("Usage: killall <name> {sig}.\n");
        return -1;
    }

    const char* target = argv[1];
    if(argc > 2)
        sig = atoi(argv[2]);

    int num = syscall0(SYS_GET_PROCS_NUM);
    if(num <= 0) {
        printf("Error, can not get process list!\n");
        return -1;
    }

    procinfo_t* procs = (procinfo_t*)malloc(sizeof(procinfo_t)*num);
    if(procs == NULL) {
        printf("Error, out of memory!\n");
        return -1;
    }

    int got = syscall2(SYS_GET_PROCS, (ewokos_addr_t)num, (ewokos_addr_t)procs);
    if(got < 0) {
        printf("Error, can not get process list!\n");
        free(procs);
        return -1;
    }
    num = got;

    int self = getpid();
    int killed = 0;
    char name[PROC_INFO_MAX_CMD_LEN];

    for(int i=0; i<num; i++) {
        procinfo_t* proc = &procs[i];

        if(proc->type != TASK_TYPE_PROC) //threads die with their process
            continue;
        if(proc->pid == self) //never kill ourselves
            continue;

        get_proc_name(proc, name, sizeof(name));
        if(strcmp(name, target) != 0)
            continue;

        if(kill(proc->pid, sig) == 0) {
            printf("Killed %s (pid %d).\n", name, proc->pid);
            killed++;
        }
        else {
            printf("Error, can not kill %s (pid %d)!\n", name, proc->pid);
        }
    }

    free(procs);

    if(killed == 0) {
        printf("No matching process found: %s.\n", target);
        return -1;
    }
    return 0;
}
