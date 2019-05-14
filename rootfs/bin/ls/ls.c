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

	fs_info_t dir_info;
	if(fs_finfo(name, &dir_info) != 0 || dir_info.type != FS_TYPE_DIR) {
		tstr_free(fname);
			return -1;
	}

	int fd = open(name, O_RDONLY);
	if(fd >= 0) {
		printf("  NAME                     TYPE  OWNER  SIZE\n");
		uint32_t i = 0;
		while(i < dir_info.size) {
			fs_info_t info;
			if(fs_kid(fd, i, &info) != 0)
				break;

			tstr_t* name = vfs_short_name_by_node(info.node);
			if(name != NULL) {
				if(info.type == FS_TYPE_FILE)
					printf("  %24s  f    %4d   %d\n", CS(name), info.owner, info.size);
				else if(info.type == FS_TYPE_DIR)
					printf("  %24s  d    %4d   %d\n", CS(name), info.owner, info.size);
				tstr_free(name);
			}
			i++;
		}
		close(fd);
	}
	tstr_free(fname);
	return 0;
}
