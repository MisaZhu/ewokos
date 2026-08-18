#include <time.h>
#include <string.h>
#include <stddef.h>

/*
 * Minimal strptime supporting the directives commonly used by media and
 * network code: date/time numbers, abbreviated/full month and weekday
 * names, and the composite %T/%R/%D/%F forms.
 */

static const char *month_names[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

static const char *wday_names[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	"Saturday"
};

static int is_space_c(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
		c == '\v' || c == '\f';
}

static const char *parse_num(const char *s, int min_digits, int max_digits,
		int *out) {
	int value = 0;
	int digits = 0;

	while (*s >= '0' && *s <= '9' && digits < max_digits) {
		value = value * 10 + (*s - '0');
		s++;
		digits++;
	}
	if (digits < min_digits)
		return NULL;
	*out = value;
	return s;
}

static const char *parse_name(const char *s, const char *names[], int count,
		int *out) {
	for (int i = 0; i < count; i++) {
		const char *n = names[i];
		const char *p = s;
		const char *q = n;
		int matched = 0;

		/* Match full name or 3-letter abbreviation, case-insensitive. */
		while (*p != '\0' && *q != '\0') {
			char a = *p;
			char b = *q;
			if (a >= 'A' && a <= 'Z')
				a += 'a' - 'A';
			if (b >= 'A' && b <= 'Z')
				b += 'a' - 'A';
			if (a != b)
				break;
			p++;
			q++;
			matched++;
		}
		if (matched >= 3 && (*q == '\0' || matched == (int)strlen(n) ||
					matched == 3)) {
			*out = i;
			return p;
		}
	}
	return NULL;
}

char *strptime(const char *s, const char *format, struct tm *tm) {
	if (s == NULL || format == NULL || tm == NULL)
		return NULL;

	while (*format != '\0') {
		if (*format != '%') {
			if (is_space_c(*format)) {
				while (is_space_c(*s))
					s++;
				format++;
				continue;
			}
			if (*s != *format)
				return NULL;
			s++;
			format++;
			continue;
		}

		format++; /* skip '%' */
		switch (*format) {
		case 'Y': {
			int v;
			s = parse_num(s, 2, 4, &v);
			if (s == NULL)
				return NULL;
			tm->tm_year = v - 1900;
			break;
		}
		case 'y': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL)
				return NULL;
			tm->tm_year = (v >= 69) ? v : v + 100;
			break;
		}
		case 'm': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v < 1 || v > 12)
				return NULL;
			tm->tm_mon = v - 1;
			break;
		}
		case 'd':
		case 'e': {
			int v;
			while (*s == ' ')
				s++;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v < 1 || v > 31)
				return NULL;
			tm->tm_mday = v;
			break;
		}
		case 'H': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v > 23)
				return NULL;
			tm->tm_hour = v;
			break;
		}
		case 'I': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v < 1 || v > 12)
				return NULL;
			tm->tm_hour = (v == 12) ? 0 : v;
			break;
		}
		case 'M': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v > 59)
				return NULL;
			tm->tm_min = v;
			break;
		}
		case 'S': {
			int v;
			s = parse_num(s, 1, 2, &v);
			if (s == NULL || v > 60)
				return NULL;
			tm->tm_sec = v;
			break;
		}
		case 'j': {
			int v;
			s = parse_num(s, 1, 3, &v);
			if (s == NULL || v < 1 || v > 366)
				return NULL;
			tm->tm_yday = v - 1;
			break;
		}
		case 'b':
		case 'h':
		case 'B': {
			int v;
			s = parse_name(s, month_names, 12, &v);
			if (s == NULL)
				return NULL;
			tm->tm_mon = v;
			break;
		}
		case 'a':
		case 'A': {
			int v;
			s = parse_name(s, wday_names, 7, &v);
			if (s == NULL)
				return NULL;
			tm->tm_wday = v;
			break;
		}
		case 'p': {
			if ((*s == 'A' || *s == 'a') &&
					(s[1] == 'M' || s[1] == 'm')) {
				if (tm->tm_hour == 12)
					tm->tm_hour = 0;
				s += 2;
			} else if ((*s == 'P' || *s == 'p') &&
					(s[1] == 'M' || s[1] == 'm')) {
				if (tm->tm_hour >= 1 && tm->tm_hour <= 11)
					tm->tm_hour += 12;
				s += 2;
			} else {
				return NULL;
			}
			break;
		}
		case 'T': {
			char *r = strptime(s, "%H:%M:%S", tm);
			if (r == NULL)
				return NULL;
			s = r;
			break;
		}
		case 'R': {
			char *r = strptime(s, "%H:%M", tm);
			if (r == NULL)
				return NULL;
			s = r;
			break;
		}
		case 'D': {
			char *r = strptime(s, "%m/%d/%y", tm);
			if (r == NULL)
				return NULL;
			s = r;
			break;
		}
		case 'F': {
			char *r = strptime(s, "%Y-%m-%d", tm);
			if (r == NULL)
				return NULL;
			s = r;
			break;
		}
		case 'n':
		case 't':
			while (is_space_c(*s))
				s++;
			break;
		case '%':
			if (*s != '%')
				return NULL;
			s++;
			break;
		case '\0':
			return NULL;
		default:
			/* Unknown directive: try to match it literally. */
			if (*s != *format)
				return NULL;
			s++;
			break;
		}
		format++;
	}

	return (char *)s;
}
