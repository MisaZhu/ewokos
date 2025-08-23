#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <ewoksys/vfs.h>
#include <tinyjson/tinyjson.h>

int main(int argc, char** argv) {
	if(argc < 2)  {
		printf("Usage: json <file>\n");
		return -1;
	}

	int sz = 0;
	char* str = (char*)vfs_readfile(argv[1], &sz);
	if(str == NULL) {
		printf("Error: %s open failed\n", argv[1]);
		return -1;
	}

	str[sz] = 0;

	json_var_t* var = json_parse_str(str);
	free(str);

	if(var == NULL) {
		printf("Error: json parse failed\n");
		return -1;
	}

	str_t* mstr = str_new(NULL);
	json_var_to_json_str(var, mstr, 0);
	printf("%s\n", mstr->cstr);
	str_free(mstr);

	json_var_t* v = json_find_var(var, "/kid/sister/name");
	if(v != NULL)
		printf("%s\n", json_var_get_str(v));

	json_var_unref(var);
	return 0;
}
