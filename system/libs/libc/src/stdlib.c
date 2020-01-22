#include <stdlib.h>
#include <sys/syscall.h>
#include <string.h>

void *malloc(size_t size) {
	return (void*)syscall1(SYS_MALLOC, (int32_t)size);
}

void free(void* ptr) {
	syscall1(SYS_FREE, (int32_t)ptr);
}

void* realloc_raw(void* s, uint32_t old_size, uint32_t new_size) {
	void* p = malloc(new_size);
	memcpy(p, s, old_size);
	free(s);
	return p;
}

void exit(int status) {
	syscall1(SYS_EXIT, (int32_t)status);
}

int execl(const char* fname, const char* arg, ...) {
	(void)fname; //TODO
	(void)arg; //TODO

	return 0;
}

const char* getenv(const char* name) {
	static char ret[1024];
	syscall3(SYS_PROC_GET_ENV, (int32_t)name, (int32_t)ret, 1023);
	return ret;
}

int setenv(const char* name, const char* value) {
	return syscall2(SYS_PROC_SET_ENV, (int32_t)name, (int32_t)value);
}

int atoi_base(const char *s, int b) {
	int i, result, x, error;
	for (i = result = error = x = 0; s[i]!='\0'; i++, result += x) {
		if (b==2) {
			if (!(s[i]>47&&s[i]<50)){			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==8) {
			if (i==0 && s[i]=='0'){
				x=0;
			}
			else if (!(s[i]>47&&s[i]<56)) {	//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==10) {
			if (!(s[i]>47&&s[i]<58)) {			//rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==16) {
			if((i==0 && s[i]=='0')||(i==1 && (s[i]=='X'||s[i]=='x'))) {
				x = 0;
			}
			else if (!((s[i]>47 && s[i]<58) || ((s[i]>64 && s[i]<71) || (s[i]>96 && s[i]<103)) )) {		//rango
				error = 1;
			}
			else {
				x = (s[i]>64 && s[i]<71)? s[i]-'7': s[i] - '0';
				x = (s[i]>96 && s[i]<103)? s[i]-'W': x;
				result *= b;
			}
		}
	}

	if (error)
		return 0;
	else
		return result;
}

int atoi(const char *str) {
	return atoi_base(str, 10);
}

float atof(const char* s) {
	(void)s;
#ifdef M_FLOAT
	return 0.0f; //TODO
#else
	return 0.0f;
#endif
}
