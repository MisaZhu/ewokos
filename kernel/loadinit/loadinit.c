#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/kernel.h>
#include <mm/kmalloc.h>
#include <kstring.h>
#include <kprintf.h>

#define KFS_ITEM_MAX 256
#define FNAME_MAX 256
#define KFS_TYPE_DIR  0
#define KFS_TYPE_FILE 1

typedef struct {
	char name[FNAME_MAX];
	uint32_t type;
	char* data;
	uint32_t size;
} romfs_item_t;

romfs_item_t _romfs_items[KFS_ITEM_MAX];
uint32_t _romfs_item_num = 0;

static void add_file(const char* name, char* p, int32_t size) {
	if(_romfs_item_num >= KFS_ITEM_MAX)
		return;

	romfs_item_t* f = (romfs_item_t*)&_romfs_items[_romfs_item_num];
	memset(f, 0, sizeof(romfs_item_t));
	strncpy(f->name, name, FNAME_MAX-1);
	f->type = KFS_TYPE_FILE;
	f->size = size;
	f->data = p;
	_romfs_item_num++;
}

static int32_t add_dir(const char* name) {
	if(_romfs_item_num >= KFS_ITEM_MAX)
		return -1;

	romfs_item_t* f = (romfs_item_t*)&_romfs_items[_romfs_item_num];
	memset(f, 0, sizeof(romfs_item_t));
	strncpy(f->name, name, FNAME_MAX-1);
	f->type = KFS_TYPE_DIR;
	f->size = 0;
	f->data = NULL;
	_romfs_item_num++;
	return 0;
}

extern const char* romfs_data[];

static void b16_decode(const char *input, uint32_t input_len, char *output, uint32_t *output_len) {
	uint32_t i;
	for (i = 0; i < input_len / 2; i++) {
		uint8_t low = input[2 * i] - 'A';
		uint8_t high = input[2 * i + 1] - 'A';
		output[i] = low | (high << 4);
	}
	*output_len = input_len / 2;
}

static int32_t load(char* ret, int32_t i) {
	while(1) {
		const char* s = romfs_data[i++];
		if(s == NULL)
			break;
		uint32_t sz = 0;
		b16_decode(s, strlen(s), ret , &sz);
		ret += sz;
	}
	return i;
}

static int32_t atoi_base(const char *s, int32_t b) {
	int32_t i, result, x, error;
	for (i = result = error = x = 0; s[i]!='\0'; i++, result += x) {
		if (b==2) {
			if (!(s[i]>47&&s[i]<50)){     //rango
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
			else if (!(s[i]>47&&s[i]<56)) { //rango
				error = 1;
			}
			else {
				x = s[i] - '0';
				result *= b;
			}
		}
		else if (b==10) {
			if (!(s[i]>47&&s[i]<58)) {      //rango
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
			else if (!((s[i]>47 && s[i]<58) || ((s[i]>64 && s[i]<71) || (s[i]>96 && s[i]<103)) )) {   //rango
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

static int32_t add_nodes(int32_t i) {
	while(1) {
		//read name
		const char* s = romfs_data[i++];
		if(s == NULL)
			return i;

		const char* sz = romfs_data[i++];
		if(sz[0] == 'r') { //dir type
			add_dir(s);
			i = add_nodes(i);
			add_file("", NULL, 0); //empty file means end of dir
		}
		else {
			int32_t size = atoi_base(sz, 10);
			char* data = NULL;
			if(size > 0) {
				data = (char*)kmalloc(size);
			}
			i = load(data, i);
			add_file(s, data, size);
		}
	}
	return i;
}

int32_t romfs_get(uint32_t index, char* name, char* data) {
	if(index >= _romfs_item_num)
		return -1;

	if(name != NULL)
		strcpy(name, _romfs_items[index].name);
	if(data != NULL && _romfs_items[index].data != NULL && _romfs_items[index].size > 0)
		memcpy(data, _romfs_items[index].data, _romfs_items[index].size);
	return _romfs_items[index].size;
}

static char* romfs_get_by_name(const char* fname, int32_t *size) {
  int32_t index = 0;
	*size = 0;
  char* data = NULL;

  while(true) {
    char name[FNAME_MAX+1];
    int32_t sz = romfs_get(index, name, NULL);
    if(sz < 0)
      break;
    if(sz > 0 && name[0] != 0 && strcmp(fname, name) == 0)  {
      data = (char*)kmalloc(sz);
      sz = romfs_get(index, NULL, data);
      *size = sz;
      return data;
    }
    index++;
  }
  return NULL;
}

int32_t load_init_proc(void) {
	_romfs_item_num = 0;
	add_nodes(0);
	const char* prog = "/sbin/init";

	printf("  read %s from kernel-fs\n", prog);
  int32_t init_size;
  char* elf = romfs_get_by_name(prog, &init_size);
  if(elf == NULL) {
    return -1;
  }

  proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
	proc->info.owner = -1;
  strcpy(proc->info.cmd, prog);
  set_translation_table_base((uint32_t)V2P(proc->space->vm));
  int32_t res = proc_load_elf(proc, elf, init_size);
  set_translation_table_base((uint32_t)V2P(_kernel_vm));
  kfree(elf);
  return res;
}

