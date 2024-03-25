#include "fontd.h"
#include "ewoksys/mstr.h"

char* font_cmd(int from_pid, int argc, char** argv, void* p) {
	if(strcmp(argv[0], "list") == 0) {
		str_t* str = str_new("");
        uint32_t i = 0;
        for(int i=0; i<TTF_MAX; i++) {
            if(_ttfs[i].ttf != NULL) {
                str_add(str, _ttfs[i].name);
                str_add(str, ", ");
                str_add(str, _ttfs[i].fname);
                str_add(str, "\n");
            }
        }
		char* ret = str_detach(str);
		return ret;
	}
	return NULL;
}