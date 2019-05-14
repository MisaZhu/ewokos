#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	tstr_t* fname = NULL;
	if(argc < 2) {
		fname = fs_full_name("");
	}
	else {
		fname = fs_full_name(argv[1]);
	}
	const char *name = CS(fname);

	printf("  NAME                     TYPE  OWNER  SIZE\n");
	uint32_t i = 0;
	char full[FULL_NAME_MAX];
	while(true) {
		tstr_t* kid = fs_kid(name, i);
		if(kid == NULL)
			break;

		if(strcmp(name, "/") != 0)
			snprintf(full, FULL_NAME_MAX-1, "%s/%s", name, CS(kid));
		else
			snprintf(full, FULL_NAME_MAX-1, "/%s", CS(kid));
		fs_info_t info;
		fs_finfo(full, &info);
		if(info.type == FS_TYPE_FILE)
			printf("  %24s  f    %4d   %d\n", CS(kid), info.owner, info.size);
		else if(info.type == FS_TYPE_DIR)
			printf("  %24s  d    %4d   %d\n", CS(kid), info.owner, info.size);
		tstr_free(kid);
		i++;
	}
	tstr_free(fname);
	return 0;
}
