#include <stdio.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>

int pids[MAX_CORE_NUM][MAX_SCHD_TRACE_NUM];
int traces;

int main (int argc, char **argv) {
  sys_info_t sys_info;
  sys_get_sys_info(&sys_info);

  while(true) {
    traces = syscall1(SYS_GET_TRACE, (int)pids);
    printf("%d\n", traces);
    for(uint32_t trace=0; trace<traces; trace++) {
      for(uint32_t core=0; core<sys_info.cores; core++) {
        printf("%8d - %d:%d ", trace, core, pids[core][trace]);
      }
      printf("\n");
    }
    usleep(500000);
  }
  return 0;
}