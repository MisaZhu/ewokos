#include <time.h>
#include <sys/errno.h>
#include <ewoksys/kernel_tic.h>

int clock_getres(clockid_t clock_id, struct timespec *res) {
	if (res == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (clock_id != CLOCK_REALTIME && clock_id != CLOCK_MONOTONIC) {
		errno = EINVAL;
		return -1;
	}

	res->tv_sec = 0;

	if (clock_id == CLOCK_MONOTONIC) {
		uint32_t hz = kernel_tic_fine_hz();
		if (hz != 0) {
			/*
			 * The interpolated clock cannot resolve anything finer than
			 * one count of the counter it is interpolated from (24MHz on
			 * the aarch64 virt target -> 41ns). Claiming 1us here, as
			 * this used to, hid the real limit; claiming 1ns would be a
			 * lie in the other direction.
			 */
			res->tv_nsec = (long)(1000000000ULL / hz);
			if (res->tv_nsec < 1L)
				res->tv_nsec = 1L;
			return 0;
		}
		/*
		 * No user-readable counter (lego.ev3, x86): CLOCK_MONOTONIC only steps
		 * once per scheduler tick. tick_usec is measured by the kernel rather
		 * than derived from timer_freq because some BSPs ignore that setting.
		 */
		uint32_t tick_usec = kernel_tic_tick_usec();
		if (tick_usec == 0)
			tick_usec = 1000;   /* nothing published yet; assume the default */
		res->tv_nsec = (long)tick_usec * 1000L;
		return 0;
	}

	/* CLOCK_REALTIME comes from gettimeofday(), a microsecond source. */
	res->tv_nsec = 1000L;
	return 0;
}
