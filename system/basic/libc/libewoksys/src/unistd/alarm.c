#include <unistd.h>
#include <errno.h>

/* No signal delivery for timers: alarm() is a no-op. */
unsigned int alarm(unsigned int seconds) {
    (void)seconds;
    return 0;
}

/* Without real signals pause() can only sleep until interrupted. */
int pause(void) {
    for (;;) {
        usleep(1000 * 1000);
    }
    errno = EINTR;
    return -1;
}
