#ifndef ELF_DYN_H
#define ELF_DYN_H

/*
 * Dynamic-linking ELF definitions.
 *
 * elf/elf.h only describes the static image (header + program headers) that
 * proc_load_elf() consumes, and its elf_type enum stops at EXECUTABLE. A
 * shared object is ET_DYN and carries a PT_DYNAMIC segment full of DT_* tags
 * plus relocation tables, none of which are described there. This header adds
 * exactly that, so libdl can parse a .so without duplicating the whole ELF
 * ABI. All structures are the on-disk layouts; both classes are provided
 * because EwokOS builds ELFCLASS_32 (arm/x86/riscv32) and ELFCLASS_64
 * (aarch64/x86_64/riscv64) targets from one source tree.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* elf_type value missing from elf/elf.h's enum */
#define ELFTYPE_SHARED          3

/* program header types */
#define PT_NULL                 0
#define PT_LOAD                 1
#define PT_DYNAMIC              2
#define PT_INTERP               3
#define PT_NOTE                 4
#define PT_SHLIB                5
#define PT_PHDR                 6
#define PT_TLS                  7

/* program header flags */
#define PF_X                    0x1
#define PF_W                    0x2
#define PF_R                    0x4

/* DT_* dynamic tags */
#define DT_NULL                 0
#define DT_NEEDED               1
#define DT_PLTRELSZ             2
#define DT_PLTGOT               3
#define DT_HASH                 4
#define DT_STRTAB               5
#define DT_SYMTAB               6
#define DT_RELA                 7
#define DT_RELASZ               8
#define DT_RELAENT              9
#define DT_STRSZ                10
#define DT_SYMENT               11
#define DT_INIT                 12
#define DT_FINI                 13
#define DT_SONAME               14
#define DT_RPATH                15
#define DT_SYMBOLIC             16
#define DT_REL                  17
#define DT_RELSZ                18
#define DT_RELENT               19
#define DT_PLTREL               20
#define DT_DEBUG                21
#define DT_TEXTREL              22
#define DT_JMPREL               23
#define DT_BIND_NOW             24
#define DT_INIT_ARRAY           25
#define DT_FINI_ARRAY           26
#define DT_INIT_ARRAYSZ         27
#define DT_FINI_ARRAYSZ         28
#define DT_RUNPATH              29
/* The bare-metal GNU ld emits only the SysV hash, but a .so built elsewhere
   may carry the GNU one; recognise the tag so it can be rejected loudly
   instead of being mistaken for an unknown-and-ignorable entry. */
#define DT_GNU_HASH             0x6ffffef5ull
#define DT_VERSYM               0x6ffffff0ull
#define DT_RELACOUNT            0x6ffffff9ull
#define DT_RELCOUNT             0x6ffffffaull

/* symbol binding, type, visibility, section index */
#define STB_LOCAL               0
#define STB_GLOBAL              1
#define STB_WEAK                2

#define STT_NOTYPE              0
#define STT_OBJECT              1
#define STT_FUNC                2
#define STT_SECTION             3
#define STT_FILE                4
#define STT_COMMON              5
#define STT_TLS                 6

#define STV_DEFAULT             0
#define STV_HIDDEN              2

#define SHN_UNDEF               0
#define SHN_ABS                 0xfff1
#define SHN_COMMON              0xfff2

#define ELF32_ST_BIND(i)        ((i) >> 4)
#define ELF32_ST_TYPE(i)        ((i) & 0xf)
#define ELF64_ST_BIND(i)        ((i) >> 4)
#define ELF64_ST_TYPE(i)        ((i) & 0xf)

#define ELF32_R_SYM(i)          ((uint32_t)(i) >> 8)
#define ELF32_R_TYPE(i)         ((uint32_t)(i) & 0xff)
#define ELF64_R_SYM(i)          ((uint64_t)(i) >> 32)
#define ELF64_R_TYPE(i)         ((uint64_t)(i) & 0xffffffffull)

typedef struct {
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t  st_info;
	uint8_t  st_other;
	uint16_t st_shndx;
} Elf32_Sym;

/*
 * st_name is an Elf64_Word, NOT a 64-bit field - the "64" in Elf64_Sym refers
 * to st_value/st_size, and the ABI fixes this struct at exactly 24 bytes so a
 * loader can stride .dynsym by DT_SYMENT. Declaring st_name as uint64_t pads
 * the struct to 32 and silently shifts st_info/st_shndx/st_value onto the wrong
 * bytes of every entry, which turns a defined symbol into SHN_UNDEF and its
 * address into its size. The DL_ASSERT_SIZE checks below exist because that
 * mistake compiles cleanly and only shows up as "undefined symbol" for symbols
 * the object plainly defines.
 */
typedef struct {
	uint32_t st_name;
	uint8_t  st_info;
	uint8_t  st_other;
	uint16_t st_shndx;
	uint64_t st_value;
	uint64_t st_size;
} Elf64_Sym;

typedef struct {
	uint32_t d_tag;
	uint32_t d_val;
} Elf32_Dyn;

typedef struct {
	uint64_t d_tag;
	uint64_t d_val;
} Elf64_Dyn;

/* REL carries the addend in place at r_offset; RELA carries it explicitly */
typedef struct {
	uint32_t r_offset;
	uint32_t r_info;
} Elf32_Rel;

typedef struct {
	uint32_t r_offset;
	uint32_t r_info;
	int32_t  r_addend;
} Elf32_Rela;

typedef struct {
	uint64_t r_offset;
	uint64_t r_info;
} Elf64_Rel;

typedef struct {
	uint64_t r_offset;
	uint64_t r_info;
	int64_t  r_addend;
} Elf64_Rela;

/* SysV hash table: nbucket, nchain, then nbucket+nchain uint32 words */
typedef struct {
	uint32_t nbucket;
	uint32_t nchain;
} Elf_Hash_Header;

/*
 * Compile-time check that every struct above has the size the ELF ABI fixes
 * for it. A loader strides these tables by a byte count taken from the file
 * (DT_SYMENT, DT_RELAENT), so a struct that is merely one field too wide reads
 * neighbouring entries' bytes instead of faulting - the failure surfaces much
 * later as a bogus "undefined symbol". The negative-array-size trick is C99,
 * which is what the whole tree builds with; file scope keeps it out of
 * -Wunused-local-typedefs.
 */
#define ELF_DYN_SIZE_CHECK(tag, type, expect) \
	typedef char tag[(sizeof(type) == (expect)) ? 1 : -1]

ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_sym32,  Elf32_Sym,        16);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_sym64,  Elf64_Sym,        24);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_dyn32,  Elf32_Dyn,         8);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_dyn64,  Elf64_Dyn,        16);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_rel32,  Elf32_Rel,         8);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_rela32, Elf32_Rela,       12);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_rel64,  Elf64_Rel,        16);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_rela64, Elf64_Rela,       24);
ELF_DYN_SIZE_CHECK(elf_dyn_sizechk_hash,   Elf_Hash_Header,   8);

/* --- AArch64 (AArch64 ELF ABI) --- */
#define R_AARCH64_NONE          256
#define R_AARCH64_ABS64         257
#define R_AARCH64_ABS32         258
#define R_AARCH64_ABS16         259
#define R_AARCH64_PREL64        260
#define R_AARCH64_PREL32        261
#define R_AARCH64_COPY          1024
#define R_AARCH64_GLOB_DAT      1025
#define R_AARCH64_JUMP_SLOT     1026
#define R_AARCH64_RELATIVE      1027
#define R_AARCH64_TLS_DTPMOD64  1028
#define R_AARCH64_TLS_DTPREL64  1029
#define R_AARCH64_TLS_TPREL64   1030
#define R_AARCH64_TLSDESC       1031
#define R_AARCH64_JUMP26        282
#define R_AARCH64_CALL26        283

/* --- ARM 32-bit (AAELF) --- */
#define R_ARM_NONE              0
#define R_ARM_PC24              1
#define R_ARM_ABS32             2
#define R_ARM_COPY              20
#define R_ARM_GLOB_DAT          21
#define R_ARM_JUMP_SLOT         22
#define R_ARM_RELATIVE          23
#define R_ARM_CALL28            28
#define R_ARM_JUMP24            29
#define R_ARM_PREL31            42
#define R_ARM_TARGET1           38

/* --- i386 --- */
#define R_386_NONE              0
#define R_386_32                1
#define R_386_PC32              2
#define R_386_COPY              5
#define R_386_GLOB_DAT          6
#define R_386_JMP_SLOT          7
#define R_386_RELATIVE          8

/* --- x86_64 --- */
#define R_X86_64_NONE           0
#define R_X86_64_64             1
#define R_X86_64_PC32           2
#define R_X86_64_COPY           5
#define R_X86_64_GLOB_DAT       6
#define R_X86_64_JUMP_SLOT      7
#define R_X86_64_RELATIVE       8

/* --- RISC-V --- */
#define R_RISCV_NONE            0
#define R_RISCV_32              1
#define R_RISCV_64              2
#define R_RISCV_RELATIVE        3
#define R_RISCV_COPY            4
#define R_RISCV_JUMP_SLOT       5

#ifdef __cplusplus
}
#endif

#endif
