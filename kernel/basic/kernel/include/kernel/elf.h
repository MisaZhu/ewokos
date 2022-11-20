#ifndef ELF_H
#define ELF_H

#include <stdint.h>

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

struct elf_ident{
	uint32_t magic;
	uint8_t  class;
	uint8_t  data;
	uint8_t  version;
	uint8_t  osabi;
	uint8_t  abiversion;
	uint8_t  pad;
	uint8_t  resv[4]; 
};

struct elf_header {
	struct elf_ident ident;
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

struct elf64_header {
	struct elf_ident ident;
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
};

struct elf64_program_header {
  uint32_t type;
  uint32_t flags;
  uint64_t off;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;
};

#define ELF_CLASS(img)			(((struct elf_header*)img)->ident.class)
#define ELF_TYPE(img) 			(((struct elf_header*)img)->type)
#define ELF_PHOFF(img) 			(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_header*)img)->phoff): \
									(((struct elf64_header*)img)->phoff))
#define ELF_PHNUM(img) 			(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_header*)img)->phnum): \
									(((struct elf64_header*)img)->phnum))
#define ELF_ENTRY(img) 			(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_header*)img)->entry): \
									(((struct elf64_header*)img)->entry))
#define ELF_PHEAD(img, n)		((uint64_t)img + ELF_PHOFF(img) + (ELF_CLASS(img) == ELFCLASS_32? \
									(sizeof(struct elf_program_header) * n): \
									(sizeof(struct elf64_program_header) * n)))
#define ELF_PVADDR(img, n)		(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_program_header*)(ELF_PHEAD(img, n)))->vaddr): \
									(((struct elf64_program_header*)(ELF_PHEAD(img, n)))->vaddr))
#define ELF_PSIZE(img, n)		(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_program_header*)(ELF_PHEAD(img, n)))->memsz): \
									(((struct elf64_program_header*)(ELF_PHEAD(img, n)))->memsz))
#define ELF_POFFSET(img, n)		(ELF_CLASS(img) == ELFCLASS_32? \
									(((struct elf_program_header*)(ELF_PHEAD(img, n)))->off): \
									(((struct elf64_program_header*)(ELF_PHEAD(img, n)))->off))

#endif
