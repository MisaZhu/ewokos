#include "mbc.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define MAGIC_NO 0x19760427
#define VERSION  0x00000001

static bool gen_mbc(int fd, vm_t* vm) {
	PC i;

	i = MAGIC_NO;
	if(write(fd, (uint32_t*)&i, 4) != 4)
		return false;

	i = VERSION;
	if(write(fd, (uint32_t*)&i, 4) != 4)
		return false;

	PC sz = vm->bc.mstr_table.size;

	if(write(fd, (uint32_t*)&sz, 4) != 4)
		return false;

	for(i=0; i<sz; ++i) {
		const char* str = (const char*)vm->bc.mstr_table.items[i];
		uint32_t len = strlen(str);
		if(write(fd, &len, 4) != 4)
			return false;
		if(write(fd, str, len) != len)
			return false;
	}

	sz = vm->bc.cindex * 4;
	if(write(fd, (uint32_t*)&sz, 4) != 4)
		return false;
	char* p = (char*)vm->bc.code_buf;
	while(sz > 0) {
		int wr = write(fd, p, sz);
		if(wr <= 0 || sz < wr)
			return false;
		p += wr;
		sz -= wr; 
	}
	return true;
}

void vm_gen_mbc(vm_t* vm, const char* fname_out) {
	if(fname_out[0] == 0)
		fname_out = "out.mbc";

	int fd = open(fname_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd < 0)
		return;

	gen_mbc(fd, vm);
	close(fd);
}

static bool load_mbc(int fd, vm_t* vm) {
	PC i, sz;
	uint32_t version;

	if(read(fd, &i, 4) != 4)
		return false;
	if(i != MAGIC_NO)
		return false;

	if(read(fd, &i, 4) != 4)
		return false;
	version = i;

	if(read(fd, &sz, 4) != 4)
		return false;
	
	for(i=0; i<sz; ++i) {
		uint32_t len;
		if(read(fd, &len, 4) != 4)
			return false;
		char* s = (char*)_malloc(len+1);
		if(s == NULL)
			return false;
		if(read(fd, s, len) != len) {
			_free(s);
			return false;
		}		
		s[len] = 0;
		array_add(&vm->bc.mstr_table, s);
		//_free(s);
	}

	if(read(fd, &sz, 4) != 4)
		return false;
	vm->bc.code_buf = (PC*)_malloc(sz);
	if(vm->bc.code_buf == NULL)
		return false;

	char* p = (char*)vm->bc.code_buf;
	vm->bc.cindex = sz/4;
	while(sz > 0) {
		int rd = read(fd, p, sz);
		if(rd <= 0 || sz < rd)
			return false;
		p += rd;
		sz -= rd; 
	}
	return true;
}

bool vm_load_mbc(vm_t* vm, const char* fname) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0)
		return false;

	bool ret = load_mbc(fd, vm);
	close(fd);
	return ret;
}

