#include <stdlib.h>
#include <syscall.h>

void* malloc(uint32_t size) {
	char* p = (char*)syscall1(SYSCALL_PMALLOC, size);
	return (void*)p;
}

void free(void* p) {
	syscall1(SYSCALL_PFREE, (int)p);
}

static int32_t isDigit(char c) {
	return (c >= '0') && (c <= '9');
}

int32_t atoi(const char *str) {
	int32_t result = 0;
	int32_t neg_multiplier = 1;

	// Check for negative
	if (*str && *str == '-') {
		neg_multiplier = -1;
		str++;
	}

	// Do number
	for (; *str && isDigit(*str); str++) {
		result = (result * 10) + (*str - '0');
	}

	return result * neg_multiplier;
}

const char* getenv(const char* name) {
	static char ret[64];
	syscall3(SYSCALL_GET_ENV, (int32_t)name, (int32_t)ret, 63);
	return ret;
}

int32_t setenv(const char* name, const char* value) {
	return syscall2(SYSCALL_SET_ENV, (int32_t)name, (int32_t)value);
}


