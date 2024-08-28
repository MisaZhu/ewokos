#include <stdio.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>

int *pids;
int traces;

int main (int argc, char **argv) {
  sys_info_t sys_info;
  sys_get_sys_info(&sys_info);
  int fps = syscall0(SYS_GET_TRACE_FPS);
  pids = (int*)malloc(sys_info.cores * fps * 4);

  while(true) {
    traces = syscall1(SYS_GET_TRACE, (int)pids);
    printf("%d\n", traces);
    for(uint32_t trace=0; trace<traces; trace++) {
      for(uint32_t core=0; core<sys_info.cores; core++) {
        printf("%8d - %d:%d ", trace, core, pids[core*fps+trace]);
      }
      printf("\n");
    }
    usleep(500000);
  }
  return 0;
}