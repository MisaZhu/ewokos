#include <stdio.h>
#include <ewoksys/syscall.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>

int main (int argc, char **argv) {
  sys_info_t sys_info;
  sys_get_sys_info(&sys_info);
  klog("test error msg\n");
  
  return 0;
}