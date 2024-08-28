#include <elf/elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" {
#endif

int32_t elf_read_header(const char* fname, elf_header_t* header) {
	int fd = open(fname, O_RDONLY);
	if(fd < 0)
		return -1;

	int h_size = sizeof(elf_header_t);
	int size = read(fd, header, h_size);
	close(fd);

	if(size != h_size) 
		return -1;
	if(header->ident.magic != ELF_MAGIC)
		return -1;
	return 0;
}

#ifdef __cplusplus 
}
#endif