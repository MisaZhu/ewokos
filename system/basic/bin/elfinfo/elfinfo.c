#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <elf/elf.h>

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("  Usage: elfinfo <file>\n");
		return -1;
	}

	elf_header_t header;
	if(elf_read_header(argv[1], &header) != 0) {
		printf("Error: read elf header failed!\n");
		return -1;
	}

	if(header.ident.magic != ELF_MAGIC) {
		printf("Error: not elf type!\n");
		return -1;
	}
	printf("ok\n");
	return 0;
}
