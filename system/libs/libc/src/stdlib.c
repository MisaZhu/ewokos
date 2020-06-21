#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <string.h>

void *malloc(size_t size) {
	return (void*)syscall1(SYS_MALLOC, (int32_t)size);
}

void *calloc(size_t nmemb, size_t size) {
	void* p = (void*)syscall1(SYS_MALLOC, (int32_t)(nmemb*size));
	if(p != NULL)
		memset(p, 0, size);
	return p;
}

void free(void* ptr) {
	syscall1(SYS_FREE, (int32_t)ptr);
}

void* realloc(void* s, uint32_t new_size) {
	return (void*)syscall2(SYS_REALLOC, (int32_t)s, (int32_t)new_size);
}

void exit(int status) {
	syscall2(SYS_EXIT, -1, (int32_t)status);
}

int execl(const char* fname, const char* arg, ...) {
	(void)fname; //TODO
	(void)arg; //TODO

	return 0;
}

inline static int get_procd_pid(void) {
	return ipc_serv_get(IPC_SERV_PROC);
}

const char* getenv(const char* name) {
	static char ret[1024];
	ret[0] = 0;

	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, name);

	int res = ipc_call(get_procd_pid(), PROC_CMD_GET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) == 0) {
			strncpy(ret, proto_read_str(&out), 1023);
		}
	}
	PF->clear(&out);
	return ret;
}

int setenv(const char* name, const char* value) {
	proto_t in, out;
	PF->init(&out, NULL, 0);
	PF->init(&in, NULL, 0)->adds(&in, name)->adds(&in, value);

	int res = ipc_call(get_procd_pid(), PROC_CMD_SET_ENV, &in, &out);
	PF->clear(&in);
	if(res == 0) {
		if(proto_read_int(&out) != 0) {
			res = -1;
		}
	}
	else {
		res = -1;
	}
	PF->clear(&out);
	return res;
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

uint32_t random(void) {
	return random_u32();
}
