#include <stdio.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ewoksys/sys.h>
#include <sysinfo.h>
#include <fcntl.h>

int main (int argc, char **argv) {
  uint16_t chs[16];
  int fd = open("/dev/adc0", O_RDONLY);
  if (fd < 0) {
    printf("open adc0 failed\n");
    return -1;
  }
  
  int j = 0;
  while (1) {
    j++;
    int sz = read(fd, chs, sizeof(chs));
    klog("adc0 %d, %d:", j, sz);
    for(int i = 0; i < 16; i++) {
      klog(" %d:%d", i, chs[i]);
    }
    klog("\n");
    proc_usleep(100000);
  }
  return 0;
}