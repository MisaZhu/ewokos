#ifndef ELF_H
#define ELF_H

#include <types.h>

#define ELF_MAGIC 0x464C457FU

enum elf_type {
	ELFTYPE_NONE        = 0,
	ELFTYPE_RELOCATABLE = 1,
	ELFTYPE_EXECUTABLE  = 2
};

struct elf_header {
	uint32_t magic;
	char elf[12];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
};

struct elf_program_header {
  uint32_t type;
  uint32_t off;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t filesz;
  uint32_t memsz;
  uint32_t flags;
  uint32_t align;
};

#endif
