#include "js.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static mstr_t* load_script_content(const char* fname) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		return NULL;
	}

	struct stat st;
	fstat(fd, &st);

	mstr_t* ret = mstr_new_by_size(st.st_size+1);

	char* p = ret->cstr;
	uint32_t sz = st.st_size;
	while(sz > 0) {
		int rd = read(fd, p, sz);
		if(rd <= 0 || sz < rd)
			break;
		p += rd;
		sz -= rd; 
	}
	close(fd);
	
	if(sz > 0) {
		mstr_free(ret);
		return NULL;
	}
	ret->cstr[st.st_size] = 0;
	ret->len = st.st_size;
	return ret;
}

#define DEF_LIBS "/usr/local/mario"

static mstr_t* include_script(vm_t* vm, const char* name) {
	const char* path = getenv("MARIO_PATH");
	if(path == NULL)
		path = DEF_LIBS;

	mstr_t* ret = load_script_content(name);
	if(ret != NULL) {
		return ret;
	}

	mstr_t* fname = mstr_new(path);
	mstr_append(fname, "/libs/");
	mstr_append(fname, _mario_lang);
	mstr_add(fname, '/');
	mstr_append(fname, name);
	
	ret = load_script_content(name);
	mstr_free(fname);
	return ret;
}

bool load_js(vm_t* vm, const char* fname) {
	_load_m_func = include_script;

	mstr_t* s = load_script_content(fname);
	if(s == NULL) {
		printf("Can not open file '%s'\n", fname);
		return false;
	}
	
	bool ret = vm_load(vm, s->cstr);
	mstr_free(s);
	return ret;
}
