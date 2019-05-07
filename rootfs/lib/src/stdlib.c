#include <stdlib.h>
#include <syscall.h>

void* malloc(uint32_t size) {
	char* p = (char*)syscall1(SYSCALL_PMALLOC, size);
	return (void*)p;
}

void free(void* p) {
	syscall1(SYSCALL_PFREE, (int)p);
}

int32_t atoi_base(const char *s, int32_t b) {
	int32_t i, result, x, error;
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

int32_t atoi(const char *str) {
	return atoi_base(str, 10);
}

const char* getenv(const char* name) {
	static char ret[1024];
	syscall3(SYSCALL_GET_ENV, (int32_t)name, (int32_t)ret, 1023);
	return ret;
}

int32_t setenv(const char* name, const char* value) {
	return syscall2(SYSCALL_SET_ENV, (int32_t)name, (int32_t)value);
}
