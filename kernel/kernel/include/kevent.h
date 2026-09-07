#ifndef KEVENT_H
#define KEVENT_H

#include <stdint.h>
#include <ewokos_config.h>

enum {
	KEV_NONE = 0,
	KEV_PROC_EXIT,
	KEV_PROC_CREATED,
	KEV_PROC_CORE_DUMP
};

/* proc exception reason carried by KEV_PROC_CORE_DUMP */
enum {
	KEV_CORE_DUMP_UNDEF = 0, /* undefined instruction abort */
	KEV_CORE_DUMP_PREFETCH,  /* prefetch/instruction abort */
	KEV_CORE_DUMP_DATA,      /* data abort */
	KEV_CORE_DUMP_OTHER
};

/*
 * Full register snapshot captured at the exception, mirroring what the kernel's
 * dump_ctx() prints. The register set is arch specific, so it lives in a union
 * and a compile-time ARCH macro (KEV_ARCH_REGS) selects the active member: one
 * shared header describes every arch, while each build only pays for the
 * registers its target actually has.
 */
typedef union {
#if defined(__aarch64__)
	struct {
		uint64_t pc;
		uint64_t spsr_el1;
		uint64_t sp;
		uint64_t lr;
		uint64_t gpr[30]; /* x0..x29 */
	} aarch64;
#elif defined(__arm__)
	struct {
		uint32_t cpsr;
		uint32_t pc;
		uint32_t sp;
		uint32_t lr;
		uint32_t gpr[13]; /* r0..r12 */
	} arm;
#elif defined(__x86_64__) || defined(__i386__)
	struct {
		uint64_t cr2;
		uint64_t trap_no;
		uint64_t err_code;
		uint64_t pc;
		uint64_t lr;
		uint64_t cs;
		uint64_t rflags;
		uint64_t sp;
		uint64_t ss;
		uint64_t gpr[15];
	} x86;
#elif defined(__riscv)
	struct {
		unsigned long pc, ra, sp, gp, tp, t0, t1, t2, s0, s1;
		unsigned long gpr[8]; /* a0..a7 */
		unsigned long s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
		unsigned long t3, t4, t5, t6;
		unsigned long sstatus, sbadaddr, scause;
	} riscv;
#else
	struct {
		uint32_t pc;
		uint32_t sp;
	} generic;
#endif
} kev_regs_t;

/* active kev_regs_t union member for the arch being compiled */
#if defined(__aarch64__)
#define KEV_ARCH_REGS aarch64
#elif defined(__arm__)
#define KEV_ARCH_REGS arm
#elif defined(__x86_64__) || defined(__i386__)
#define KEV_ARCH_REGS x86
#elif defined(__riscv)
#define KEV_ARCH_REGS riscv
#else
#define KEV_ARCH_REGS generic
#endif

/*
 * Crash snapshot handed to the core proc when a user proc dies from an
 * exception. Addresses use ewokos_addr_t so 64-bit targets (aarch64) keep the
 * full pc/sp/fault address instead of truncating to 32 bits. The proc name is
 * intentionally not carried here to keep the queued event small.
 */
typedef struct {
	int32_t       pid;
	uint32_t      core;
	uint32_t      reason;     /* KEV_CORE_DUMP_* */
	uint32_t      status;     /* arch fault status / error code */
	ewokos_addr_t fault_addr; /* faulting address (data abort) */
	ewokos_addr_t pc;
	ewokos_addr_t sp;
	kev_regs_t    regs;       /* full register dump, arch specific */
} kev_core_dump_t;

typedef struct {
	uint32_t        type;
	uint32_t        data[3];
	kev_core_dump_t core_dump; /* valid only when type == KEV_PROC_CORE_DUMP */
} kevent_t;

#endif
