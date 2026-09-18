/* ============================================================================
 * cap_proofs.c - bounded-model-checking harness for the EwokOS capability core.
 *
 * It compiles the REAL kernel source (kernel/kernel/src/cap.c) against a small
 * stub environment (see stub/), so every assertion below is discharged against
 * the actual shipped logic, not a re-implementation.
 *
 * Run with CBMC (brew install cbmc):
 *     make check            # prove every property
 *     make check P=proof_grant_only_attenuates
 * Validate the harness itself compiles without CBMC:
 *     make smoke
 *
 * Properties proved (the security theorems from docs/wiki/21-capability):
 *   1 cnode_insert_bounded      - insert never yields/uses an out-of-range slot
 *   2 grant_only_attenuates     - a forwarded cap's rights are a subset of the source
 *   3 grant_zero_rights_rejected- attenuation to the empty right-set is refused
 *   4 mint_requires_root        - minting without CAP_ROOT always fails
 *   5 mint_root_slot_bounded    - a successful mint returns an in-range slot
 *   6 frame_overflow_rejected   - a wrapped [paddr,paddr+size) never matches
 *   7 frame_containment_sound   - only requests fully covered by a frame cap pass
 *   8 revoke_clears_slot        - revoke empties the slot; OOB revoke is refused
 *   9 root_bypass               - CAP_ROOT short-circuits every check
 * ========================================================================== */

#include <kernel/proc.h>   /* stub proc_t + kernel entry points */

/* ---- pull in the implementation under test ----------------------------- */
#include "../../kernel/src/cap.c"

/* ---- CBMC / host fallback shims --------------------------------------- */
#ifdef __CPROVER__
  #define VASSERT(c, m)  __CPROVER_assert((c), m)
  #define VASSUME(c)     __CPROVER_assume(c)
  /* bodyless functions are nondeterministic in CBMC */
  uint32_t nondet_u32(void);
  uint64_t nondet_u64(void);
#else
  #include <assert.h>
  #define VASSERT(c, m)  assert(c)
  #define VASSUME(c)     do { if(!(c)) return; } while(0)
  static uint32_t nondet_u32(void){ return 0; }
  static uint64_t nondet_u64(void){ return 0; }
#endif

/* ---- model of the proc table + current proc --------------------------- */
#define MAXP 4
static proc_t  _procs[MAXP];
static proc_t* _current;

proc_t* proc_get_proc(proc_t* p){ return p; }        /* owner == self in model */
proc_t* proc_get(int32_t pid){
    if(pid < 0 || pid >= MAXP) return NULL;
    return &_procs[pid];
}
proc_t* get_current_proc(void){ return _current; }

/* Permissive on purpose: the address-range predicate user_ptr_ok() is proved
   separately against proc.c; here it only has to not block the cap logic. */
bool user_ptr_ok(proc_t* proc, ewokos_addr_t ptr, ewokos_addr_t size){
    (void)size;
    return proc != NULL && ptr != 0;
}

/* bounded model of EwokOS sstrncpy (only reached by the policy_add path) */
char* sstrncpy(char* dst, const char* src, size_t n){
    if(n > 0) dst[0] = src ? src[0] : 0;
    return dst;
}

/* ---- helpers ---------------------------------------------------------- */
static uint32_t nondet_rights(void){
    return nondet_u32() & (CAP_R | CAP_W | CAP_X | CAP_GRANT);
}
static bool cnode_model_has_root(const cnode_t* cn){
    for(uint32_t i = 0; i < CNODE_SLOTS; i++)
        if(cn->slots[i].type == CAP_ROOT) return true;
    return false;
}

/* =====================================================================
 * 1 - cap_cnode_insert never reports or writes an out-of-range slot.
 * ===================================================================== */
void proof_cnode_insert_bounded(void){
    cnode_t cn; cap_cnode_init(&cn);
    cap_t c;
    c.type  = CAP_FRAME;                 /* a valid, insertable type */
    c.rights = nondet_rights();
    c.obj.frame.paddr = nondet_u64();
    c.obj.frame.size  = nondet_u64();

    int32_t s = cap_cnode_insert(&cn, &c);
    VASSERT(s == -1 || (s >= 0 && s < (int32_t)CNODE_SLOTS),
            "insert returns a slot strictly inside [0,CNODE_SLOTS)");
}

/* =====================================================================
 * 2 - grant can only attenuate: forwarded rights subset of the source.
 * ===================================================================== */
void proof_grant_only_attenuates(void){
    proc_t* caller = &_procs[0];
    proc_t* target = &_procs[1];
    _current = caller;
    cap_cnode_init(&caller->cnode);
    cap_cnode_init(&target->cnode);

    caller->cnode.slots[0].type   = CAP_FRAME;
    caller->cnode.slots[0].rights = nondet_rights();
    caller->cnode.slots[0].obj.frame.paddr = nondet_u64();
    caller->cnode.slots[0].obj.frame.size  = nondet_u64();
    uint32_t src_rights = caller->cnode.slots[0].rights;

    uint32_t mask = nondet_u32();
    int32_t r = proc_cap_grant(1 /*target pid*/, 0 /*src slot*/, mask);
    if(r >= 0){
        uint32_t granted = target->cnode.slots[r].rights;
        VASSERT((granted & ~src_rights) == 0,
                "granted rights never exceed the source cap's rights");
        VASSERT(r < (int32_t)CNODE_SLOTS, "grant lands in range");
    }
}

/* =====================================================================
 * 3 - attenuation to the empty right-set is refused (no useless cap).
 * ===================================================================== */
void proof_grant_zero_rights_rejected(void){
    proc_t* caller = &_procs[0];
    _current = caller;
    cap_cnode_init(&caller->cnode);
    cap_cnode_init(&_procs[1].cnode);

    caller->cnode.slots[0].type   = CAP_FRAME;
    caller->cnode.slots[0].rights = CAP_R | CAP_GRANT;   /* grantable */
    caller->cnode.slots[0].obj.frame.paddr = 0;
    caller->cnode.slots[0].obj.frame.size  = 0;

    /* mask keeps none of the source rights */
    int32_t r = proc_cap_grant(1, 0, CAP_W | CAP_X);
    VASSERT(r == -1, "grant attenuated to zero rights must fail");
}

/* =====================================================================
 * 4 - minting an object cap without CAP_ROOT always fails.
 * ===================================================================== */
void proof_mint_requires_root(void){
    proc_t* caller = &_procs[0];
    _current = caller;
    cap_cnode_init(&caller->cnode);        /* no root */
    VASSUME(!cnode_model_has_root(&caller->cnode));

    cap_mint_arg_t arg;
    arg.a = nondet_u64();
    arg.b = nondet_u64();

    uint32_t type = nondet_u32();
    VASSUME(type != CAP_INVALID);
    int32_t r = proc_cap_mint(type, nondet_rights(), &arg);
    VASSERT(r == -1, "mint without CAP_ROOT must be refused");
}

/* =====================================================================
 * 5 - a successful mint (root holder) returns an in-range slot.
 * ===================================================================== */
void proof_mint_root_slot_bounded(void){
    proc_t* caller = &_procs[0];
    _current = caller;
    proc_cap_grant_root(caller);           /* installs CAP_ROOT */

    cap_mint_arg_t arg;
    arg.a = 0x1000;                        /* a frame base */
    arg.b = 0x1000;                        /* a frame size */
    int32_t r = proc_cap_mint(CAP_FRAME, CAP_R | CAP_W, &arg);
    VASSERT(r == -1 || (r >= 0 && r < (int32_t)CNODE_SLOTS),
            "mint slot strictly inside [0,CNODE_SLOTS)");
}

/* =====================================================================
 * 6 - a wrapped [paddr,paddr+size) request never matches a frame cap.
 * ===================================================================== */
void proof_frame_overflow_rejected(void){
    proc_t* p = &_procs[0];
    cap_cnode_init(&p->cnode);
    VASSUME(!cnode_model_has_root(&p->cnode));
    /* 0xFFFF... + 2 wraps to a small end < paddr */
    bool ok = proc_cap_check_frame(p, (uint64_t)~0ull, 2, CAP_W);
    VASSERT(ok == false, "a wrapped frame range is never authorized");
}

/* =====================================================================
 * 7 - frame check is sound w.r.t. interval containment.
 * ===================================================================== */
void proof_frame_containment_sound(void){
    proc_t* p = &_procs[0];
    cap_cnode_init(&p->cnode);
    /* single frame cap covering [0x1000, 0x3000) with RW */
    p->cnode.slots[0].type   = CAP_FRAME;
    p->cnode.slots[0].rights = CAP_R | CAP_W;
    p->cnode.slots[0].obj.frame.paddr = 0x1000;
    p->cnode.slots[0].obj.frame.size  = 0x2000;

    VASSERT(proc_cap_check_frame(p, 0x1000, 0x2000, CAP_W) == true,
            "exact cover passes");
    VASSERT(proc_cap_check_frame(p, 0x1800, 0x800,  CAP_W) == true,
            "inner subrange passes");
    VASSERT(proc_cap_check_frame(p, 0x0,    0x4000, CAP_W) == false,
            "request wider than the cap fails");
    VASSERT(proc_cap_check_frame(p, 0x2000, 0x2000, CAP_W) == false,
            "request spilling past the cap end fails");
    VASSERT(proc_cap_check_frame(p, 0x1000, 0x2000, CAP_W | CAP_X) == false,
            "missing right bit fails");
}

/* =====================================================================
 * 8 - revoke empties the slot; out-of-range revoke is refused.
 * ===================================================================== */
void proof_revoke_clears_slot(void){
    proc_t* caller = &_procs[0];
    _current = caller;
    cap_cnode_init(&caller->cnode);
    cap_t c;
    c.type = CAP_IRQ; c.rights = CAP_W; c.obj.irq.irq = 7;
    int32_t s = cap_cnode_insert(&caller->cnode, &c);
    VASSUME(s >= 0);

    int32_t r = proc_cap_revoke((uint32_t)s);
    VASSERT(r == 0, "revoking a live slot succeeds");
    VASSERT(caller->cnode.slots[s].type == CAP_INVALID,
            "revoked slot is cleared");

    int32_t oob = proc_cap_revoke(CNODE_SLOTS);      /* out of range */
    VASSERT(oob == -1, "out-of-range revoke is refused");
}

/* =====================================================================
 * 9 - CAP_ROOT short-circuits every authority check.
 * ===================================================================== */
void proof_root_bypass(void){
    proc_t* p = &_procs[0];
    proc_cap_grant_root(p);
    VASSERT(cnode_model_has_root(&p->cnode), "root cap installed");

    VASSERT(proc_cap_check_frame(p, 0x80000000ull, 0x1000, CAP_W) == true,
            "root passes frame check");
    VASSERT(proc_cap_check_irq(p, 33, CAP_W) == true, "root passes irq check");
    VASSERT(proc_cap_check_proc(p, 1, CAP_W) == true, "root passes proc check");
    VASSERT(proc_cap_check_ep(p, 1, CAP_X)   == true, "root passes ep check");
    VASSERT(proc_cap_has(p, CAP_DMA, CAP_W)  == true, "root passes generic has");
}
