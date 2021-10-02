#include <sys/syscall.h>

int get_cored_pid(void) {
  return syscall0(SYS_CORE_PID);
}
