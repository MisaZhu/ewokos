#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int font_load_font(const char* name, const char* fname) {
	printf("  %s ... ", name);

	int ret = font_load(name, fname);
	if(ret < 0)
		printf("[error]\n");
	else
		printf("[OK]\n");
	return ret;
}

static void font_load_config() {
	int sz = 0;
	json_var_t* var = NULL;
	char* str = (char*)vfs_readfile("/usr/system/fonts.json", &sz);
	if(str == NULL)
		return;

	str[sz] = 0;
	var = json_parse_str(str);
	free(str);
	if(var == NULL)
		return;

	uint32_t num = json_var_array_size(var);
	for(uint32_t i=0; i<num; i++) {
		json_var_t* v = json_var_array_get_var(var, i);
		const char* name = json_get_str(v, "name");
		const char* fname = json_get_str(v, "file");
		font_load_font(name, fname);
	}
	json_var_unref(var);
}

int main(int argc, char** argv) {
	font_init();
	setbuf(stdout, NULL);

    if(argc > 2) {
        if(font_load_font(argv[1], argv[2]) < 0)
            return -1;
        return 0;
    }
	else if(argc > 1) {
        if(font_load_font(NULL, argv[1]) < 0)
            return -1;
        return 0;
	}
    
    font_load_config();
    return 0;
}