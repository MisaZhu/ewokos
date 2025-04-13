#ifndef FONTD_H
#define FONTD_H

#include <freetype/freetype.h>

#define NAME_LEN 64
#define FNAME_LEN 128

typedef struct {
	char name[NAME_LEN];
	char fname[FNAME_LEN];
	FT_Face face;
} ttf_item_t;

#define TTF_MAX   8
extern ttf_item_t _ttfs[TTF_MAX];

char* font_cmd(int from_pid, int argc, char** argv, void* p);
int   font_open(const char* name, const char* fname);

#endif