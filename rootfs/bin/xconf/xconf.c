#include <unistd.h>
#include <stdio.h>
#include <graph/font.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <kstring.h>

static void help(void) {
	printf(
		"xconfole config tool\n"
		"usage: xconf <cmd> <arg> [dev_name]\n"
		"    xconf font:           list font names\n"
		"    xconf font <NAME>:    set font\n"
		"    xconf fg <HEX_COLOR>: set foreground color\n"
		"    xconf bg <HEX_COLOR>: set background color\n");
}

static void fonts(void) {
	int32_t i = 0;
	while(true) {
		font_item_t * it = get_font_by_index(i);
		if(it == NULL || it->name[0] == 0)
			break;
		if(i == 0)
			printf("  %s", it->name);
		else
			printf(", %s", it->name);
		i++;
	}
	printf("\n");
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		help();
		return -1;
	}
	else if(strcmp(argv[1], "font") == 0 && argc == 2) {
		fonts();
		return 0;
	}

	const char* dev_name = "/dev/console0";
	if(argc > 3) 
		dev_name = argv[3];
	proto_t* proto = proto_new(argv[2], strlen(argv[2])+1);
	if(strcmp(argv[1], "font") == 0) {
		fs_fctrl(dev_name, FS_CTRL_SET_FONT, proto, NULL);
	}
	else if(strcmp(argv[1], "fg") == 0) {
		fs_fctrl(dev_name, FS_CTRL_SET_FG_COLOR, proto, NULL);
	}
	else if(strcmp(argv[1], "bg") == 0) {
		fs_fctrl(dev_name, FS_CTRL_SET_BG_COLOR, proto, NULL);
	}
	proto_free(proto);
	return 0;
}
