#include <ewoksys/vdevice.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static int _font_dev_pid = -1;

int font_init(void) {
	_font_dev_pid = dev_get_pid("/dev/font");
	if(_font_dev_pid < 0)
		return -1;
	return 0;
}

static int font_load_inst(const char* name, const char* fname) {
	if(_font_dev_pid < 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "s,s", name, fname);

	int ret = -1;
	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_LOAD, &in, &out) == 0) {
		ret = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
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
	for(int i=0; i<num; i++) {
		json_var_t* v = json_var_array_get_var(var, i);
		const char* name = json_get_str(v, "name");
		const char* fname = json_get_str(v, "file");
		printf(" %s: %s ... ", name, fname);
		font_load_inst(name, fname);
		printf("[ok]\n");
	}
	json_var_unref(var);
}

int main(int argc, char** argv) {
	font_init();
	setbuf(stdout, NULL);

    if(argc > 2) {
        if(font_load_inst(argv[1], argv[2]) < 0)
            return -1;
        return 0;
    }
    
    font_load_config();
    return 0;
}