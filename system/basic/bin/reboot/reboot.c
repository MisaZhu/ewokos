#include <stdio.h>
#include <ewoksys/syscall.h>
#include <machine_conf.h>

/* Reset the board via PSCI SYSTEM_RESET (TF-A BL31).  Requires root
 * (SYS_PLATFORM_CTL is CAP_ROOT-gated).  Used by the automation pipeline to
 * reboot the board between test iterations without a manual power-cycle. */
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    printf("reboot: PSCI SYSTEM_RESET\n");
    syscall3(SYS_PLATFORM_CTL, ORIN_PLATFORM_RESET, 0, 0);
    printf("reboot: reset did not take effect (need root?)\n");
    return 1;
}