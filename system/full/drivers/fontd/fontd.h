#ifndef FONTD_H
#define FONTD_H

#include <ttf/ttf.h>

#define NAME_LEN 64
#define FNAME_LEN 128

typedef struct {
	char name[NAME_LEN];
	char fname[FNAME_LEN];
	ttf_t* ttf;
	int32_t ref;
} ttf_item_t;

#define TTF_MAX   8
extern ttf_item_t _ttfs[TTF_MAX];

char* font_cmd(int from_pid, int argc, char** argv, void* p);
void  load_config(bool basic);
int   font_open(const char* name, const char* fname);

#endif