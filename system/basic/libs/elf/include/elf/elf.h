#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

#define ELF_MAGIC 0x464C457FU

enum elf_type {
	ELFTYPE_NONE        = 0,
	ELFTYPE_RELOCATABLE = 1,
	ELFTYPE_EXECUTABLE  = 2
};


enum elf_class {
	ELFCLASS_NONE       = 0,
	ELFCLASS_32 		= 1,
	ELFCLASS_64  		= 2
};

typedef struct {
	uint32_t magic;
	uint8_t  cls;
	uint8_t  data;
	uint8_t  version;
	uint8_t  osabi;
	uint8_t  abiversion;
	uint8_t  pad;
	uint8_t  resv[4]; 
} elf_ident_t;

typedef struct {
	elf_ident_t ident;
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
} elf_header_t;

typedef struct {
  uint32_t type;
  uint32_t off;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t filesz;
  uint32_t memsz;
  uint32_t flags;
  uint32_t align;
} elf_program_header_t;

typedef struct  {
	elf_ident_t ident;
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phoff;
	uint64_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} elf64_header_t;

typedef struct {
  uint32_t type;
  uint32_t flags;
  uint64_t off;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;
} elf64_program_header_t;

int32_t elf_read_header(const char* fname, elf_header_t* header);

#ifdef __cplusplus 
}
#endif

#endif