#include <unistd.h>
#include <stdio.h>
#include <graph/font.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <kstring.h>

static void help(void) {
	printf(
		"console config tool\n"
		"usage: cons <cmd> <arg> [dev_name]\n"
		"    cons font:           list font names\n"
		"    cons font <NAME>:    set font\n"
		"    cons fg <HEX_COLOR>: set foreground color\n"
		"    cons bg <HEX_COLOR>: set background color\n");
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
	if(argc > 4) 
		dev_name = argv[4];

	if(strcmp(argv[1], "font") == 0) {
		fs_fctrl(dev_name, 1, argv[2], strlen(argv[2])+1, NULL, 0); //font cmd: 1;
	}
	else if(strcmp(argv[1], "fg") == 0) {
		fs_fctrl(dev_name, 2, argv[2], strlen(argv[2])+1, NULL, 0); //fg color cmd: 2;
	}
	else if(strcmp(argv[1], "bg") == 0) {
		fs_fctrl(dev_name, 3, argv[2], strlen(argv[2])+1, NULL, 0); //bg color cmd: 3;
	}
	return 0;
}
