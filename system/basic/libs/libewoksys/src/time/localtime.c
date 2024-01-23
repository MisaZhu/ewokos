#include <time.h>

char *
asctime(const struct tm *timeptr){
	return "0";
}

char *
asctime_r(const struct tm *restrict timeptr, char *restrict buf){
	memset(buf, '0', 8);
}

char *
ctime(const time_t *clock){
	return "0";
}

char *
ctime_r(const time_t *clock, char *buf){
	 memset(buf, '0', 8);
}

double
difftime(time_t time1, time_t time0){
	return 0;
}

struct tm *
gmtime(const time_t *clock){
	static struct tm _tm;
	return &_tm;
}

struct tm *gmtime_r(const time_t *clock, struct tm *result){
	memset(result, 0, sizeof(struct tm));
	return result;
}

struct tm *localtime(const time_t *clock){
 static struct tm _tm;
  return &_tm;
}

struct tm *localtime_r(const time_t *clock, struct tm *result){
 memset(result, 0, sizeof(struct tm));
 return result;
}

time_t mktime(struct tm *timeptr){
	static time_t _t;
	return _t;
}

time_t timegm(struct tm *timeptr){
   static time_t _t;
   return _t;
}

time_t timelocal(struct tm *timeptr){
	static time_t _t;
	return _t;
}

time_t time(time_t *_timer){
	static time_t _t;
	return _t;
}
