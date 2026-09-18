#ifndef EWOKOS_CAP_H
#define EWOKOS_CAP_H

#include <stdint.h>

/*
 * EwokOS capability model (shared kernel/userland ABI).
 *
 * Every proc_t carries a capability node (cnode) of CNODE_SLOTS capability
 * slots plus a 64-bit security id (sid) for identity checks. A capability
 * authorizes the holder to perform operations on one kernel object class,
 * attenuated by its rights mask. CAP_ROOT is the system root authority and
 * passes every capability check.
 *
 * Object references are stored BY VALUE as small descriptors instead of raw
 * kernel pointers: EwokOS kfree()s proc_t/proc_space_t at process exit, so a
 * pointer held in a cap would dangle. Process-flavoured caps therefore carry
 * (pid, uuid) and are validated against the live task table at check time,
 * which also makes them immune to pid-slot reuse.
 */

enum cap_type {
    CAP_INVALID = 0,
    CAP_AS,         /* address space cap */
    CAP_FRAME,      /* physical frame range (RAM / MMIO) */
    CAP_EP,         /* IPC endpoint */
    CAP_PROCESS,    /* process control */
    CAP_ROOT,       /* system root authority */
    CAP_IRQ,        /* interrupt */
    CAP_DMA,        /* DMA controller/block */
};

/* capability rights */
#define CAP_R       (1U << 0)
#define CAP_W       (1U << 1)
#define CAP_X       (1U << 2)
#define CAP_GRANT   (1U << 3) /* allow forwarding this cap to another process */

#define CNODE_SLOTS 64

/* object descriptors, embedded by value in cap_t */
typedef struct { int32_t pid; uint32_t uuid; }      cap_as_obj_t;   /* address space owner */
typedef struct { uint64_t paddr; uint64_t size; }   cap_frame_obj_t;/* phys range [paddr, paddr+size) */
typedef struct { int32_t pid; uint32_t uuid; }      cap_ep_obj_t;   /* ipc server */
typedef struct { int32_t pid; uint32_t uuid; }      cap_proc_obj_t; /* control target */
typedef struct { uint32_t irq; }                    cap_irq_obj_t;
typedef struct { int32_t block; }                   cap_dma_obj_t;

/*
 * Alignment note: the object union is forced to 16-byte alignment (so cap_t
 * is 32 bytes with 16-byte alignment). Compilers freely merge adjacent
 * 64-bit field accesses into single 16-byte ldp/stp/q-register ops; on this
 * kernel that is a hard requirement, because some per-core kernel SVC stacks
 * share a page mapped with device attributes (the vsyscall info page), and
 * device memory faults on ANY unaligned access regardless of SCTLR.A. With
 * the union at a naturally 16-aligned offset, every wide access the compiler
 * emits for a cap lands on a 16-aligned address.
 */
typedef struct {
    uint32_t type;   /* enum cap_type (uint32 for a stable ABI) */
    uint32_t rights; /* CAP_R/CAP_W/CAP_X/CAP_GRANT mask */
    _Alignas(16) union {
        cap_as_obj_t    as;
        cap_frame_obj_t frame;
        cap_ep_obj_t    ep;
        cap_proc_obj_t  proc;
        cap_irq_obj_t   irq;
        cap_dma_obj_t   dma;
    } obj;
} cap_t;

typedef struct {
    cap_t slots[CNODE_SLOTS];
} cnode_t;

/*
 * SYS_CAP_MINT argument (passed as a userspace pointer): type-specific
 * object arguments.
 *   CAP_FRAME:   a = physical base, b = size in bytes
 *   CAP_IRQ:     a = irq number
 *   CAP_EP:      a = server pid
 *   CAP_PROCESS: a = target pid
 *   CAP_AS:      a = owner pid
 *   CAP_DMA:     a = dma block id
 */
typedef struct {
    /* see the alignment note on cap_t: the kernel reads this as one
     * 16-byte unit, so it must never sit at an odd-8 address. */
    _Alignas(16) uint64_t a;
    uint64_t b;
} cap_mint_arg_t;

/* SYS_CAP_GET result: the queried slot plus the caller's own sid. */
typedef struct {
    cap_t    cap;
    uint64_t sid;
} cap_info_t;

/*
 * SYS_CAP_POLICY_ADD argument (passed as a userspace pointer): install one
 * entry into the kernel's exec-time capability policy. Whenever a proc
 * execs an image whose cmd (first whitespace-separated token) equals `cmd`,
 * a cap built from (type, rights, a, b) is inserted into its cnode right
 * after the exec cnode reset - so a rule takes effect no matter when the
 * process starts (or restarts). a/b follow the cap_mint_arg_t conventions.
 */
#define CAP_POLICY_CMD_MAX 256 /* keep == PROC_INFO_MAX_CMD_LEN (procinfo.h) */
typedef struct {
    /* see the alignment note on cap_t: the kernel reads a/b as 64-bit
     * units, keep them 16-aligned at the head of the struct. */
    _Alignas(16) uint64_t a;
    uint64_t b;
    uint32_t type;   /* enum cap_type; CAP_INVALID not installable; CAP_ROOT is
                        allowed (config-level delegation, see kernel cap.c) */
    uint32_t rights; /* CAP_R/CAP_W/CAP_X/CAP_GRANT mask */
    char     cmd[CAP_POLICY_CMD_MAX];
} cap_policy_add_t;

#endif
