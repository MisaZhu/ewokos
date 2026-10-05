#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <unistd.h>
#include <sys/stat.h>

static int run(const char* cmd) {
    int pid = fork();
    if(pid == 0) {
        proc_detach();

        if(proc_exec(cmd) != 0) {
            exit(-1);
        }
    }
    else if(pid > 0) {
        /*
         * The child only survives our exit once it has run proc_detach()
         * (father_pid=0); fork() parks it in BLOCK until cored clones it, so
         * bgrun can otherwise exit first and proc_terminate() SIG_STOPs the
         * still-attached child before it execs. Wait until it has detached.
         */
        procinfo_t info;
        uint32_t waited = 0;
        while(waited < 2000000) { /* 2s ceiling: detach normally lands in a few ms */
            if(proc_info(pid, &info) != 0 || info.uuid == 0)
                break;          /* child gone / slot freed */
            if(info.father_pid == 0)
                break;          /* detached: it now survives our exit */
            usleep(1000);
            waited += 1000;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {

    struct stat buf;
    if(argc< 2)
        return -1;

    //printf("run: %-42s ", argv[1]);
    printf("run: %s ", argv[1]);
    int ret = stat(argv[1], &buf);
    if(ret >= 0 && buf.st_mode & X_OK){
        str_t* cmd = str_new("");
        for(int i=1; i<argc; i++) {
            str_add(cmd, argv[i]);
            str_addc(cmd, ' ');
        }

        ret = run(cmd->cstr);
        str_free(cmd);
        printf("[\033[32m%s\033[0m]\n", "OK"); //green for ok
    }
    else 
        printf("[\033[31m%s\033[0m]\n", "ERR!"); //red for failed
    return ret;
}

