#ifndef EWOKOS_LIBC_TIME_H
#define EWOKOS_LIBC_TIME_H

#include <sys/types.h>
#include <sys/time.h>

#define CLOCKS_PER_SEC 1000000

typedef int clockid_t;

#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

#define TIMER_ABSTIME 1

#ifdef __cplusplus
extern "C" {
#endif

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

time_t time(time_t *timer);
time_t mktime(struct tm *tm);
struct tm *gmtime(const time_t *timer);
struct tm *gmtime_r(const time_t *timer, struct tm *result);
struct tm *localtime(const time_t *timer);
struct tm *localtime_r(const time_t *timer, struct tm *result);
clock_t clock(void);
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
char *strptime(const char *s, const char *format, struct tm *tm);
char *asctime(const struct tm *tm);
char *asctime_r(const struct tm *tm, char *buf);
char *ctime(const time_t *timer);
char *ctime_r(const time_t *timer, char *buf);
double difftime(time_t time1, time_t time0);
void tzset(void);
/*
 * The three POSIX timezone globals, which tzset() is specified to set.
 *
 * EwokOS has no timezone database and no TZ handling: the system clock is
 * UTC and localtime() returns UTC broken-down time.  So these are defined in
 * src/time/tzvars.c to the values that state exactly that - tzname { "UTC",
 * "UTC" }, timezone 0 (seconds west of Greenwich), daylight 0 (no DST).
 * They are real variables, not macros, because tzset() may be called and
 * because qdatetime.cpp takes their address indirectly through
 * QString::fromLocal8Bit(tzname[isDst]).
 *
 * `long timezone` does not collide with `struct timezone` in <sys/time.h>:
 * in C++ a struct name and a variable name occupy different lookup contexts,
 * and `struct timezone` is only ever spelled with the `struct` keyword.  This
 * was verified by compiling a TU that includes both headers and declares both.
 *
 * qdatetime.cpp:qt_timezone() and qt_tzname() reach these in their plain
 * #else branches - not under QT_CONFIG(timezone) - so -no-feature-timezone
 * would not have removed the errors.
 */
extern char *tzname[2];
extern long timezone;
extern int daylight;
int clock_gettime(clockid_t clock_id, struct timespec *tp);
int clock_settime(clockid_t clock_id, const struct timespec *tp);
int clock_getres(clockid_t clock_id, struct timespec *res);
int clock_nanosleep(clockid_t clock_id, int flags,
		const struct timespec *request, struct timespec *remain);
int nanosleep(const struct timespec *req, struct timespec *rem);

#ifdef __cplusplus
}
#endif

#endif
