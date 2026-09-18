#include <kernel/cap.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kstring.h>
#include <stddef.h>

/* ======================================================================
 * Capability machinery.
 *
 * Authority model
 *  - Every proc owns a cnode (CNODE_SLOTS caps) and a sid assigned at
 *    proc_create(). Threads share their owner process's authority: every
 *    check resolves the calling task to its owner proc first.
 *  - CAP_ROOT is the system root authority and bypasses every check. It is
 *    kernel-issued only (never mintable) and lands in a cnode through
 *    proc_cap_grant_root(): kernel-created procs (init, core idle) and any
 *    image exec'd while uid <= 0.
 *  - Inheritance: fork copies the parent's cnode verbatim; exec resets the
 *    cnode by uid (privileged -> root capset, user -> empty); setuid to a
 *    real user (uid > 0) drops every cap. This keeps the historical
 *    "uid > 0 fails privileged syscalls" behaviour identical while making
 *    the authority explicit and delegable.
 *
 * Delegation: a CAP_ROOT holder mints object caps (SYS_CAP_MINT) and hands
 * them out (SYS_CAP_GRANT, itself gated by the CAP_GRANT right); granted
 * rights can only be attenuated. Object caps store identities by value
 * (pid+uuid, phys range, irq, dma block) and are revalidated at check time,
 * so a cap naming a dead/recycled pid simply stops matching.
 *
 * Policy: /sbin/init parses /etc/cap.json and installs the rules with
 * SYS_CAP_POLICY_ADD. The kernel then grants a rule's caps by itself every
 * time an image whose cmd matches the rule execs - authority rides the exec
 * that creates the new identity, never a scan of already-running procs.
 * ====================================================================== */

static uint64_t _cap_sid_seq = 0;

uint64_t cap_new_sid(void) {
    /* sid 0 is reserved for "no identity"; sids are unique per boot. */
    return ++_cap_sid_seq;
}

void cap_cnode_init(cnode_t* cnode) {
    memset(cnode, 0, sizeof(cnode_t));
}

void cap_cnode_copy(cnode_t* dst, const cnode_t* src) {
    memcpy(dst, src, sizeof(cnode_t));
}

int32_t cap_cnode_insert(cnode_t* cnode, const cap_t* cap) {
    if(cnode == NULL || cap == NULL || cap->type == CAP_INVALID)
        return -1;
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        if(cnode->slots[i].type == CAP_INVALID) {
            cnode->slots[i] = *cap;
            return (int32_t)i;
        }
    }
    return -1;
}

void cap_cnode_revoke(cnode_t* cnode, uint32_t slot) {
    if(cnode == NULL || slot >= CNODE_SLOTS)
        return;
    memset(&cnode->slots[slot], 0, sizeof(cap_t));
}

static inline bool cap_rights_ok(const cap_t* c, uint32_t rights) {
    return c->type != CAP_INVALID && (c->rights & rights) == rights;
}

static bool cnode_has_root(const cnode_t* cnode) {
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        if(cnode->slots[i].type == CAP_ROOT)
            return true;
    }
    return false;
}

/* Resolve a task to the proc whose cnode authorizes its syscalls. */
static inline proc_t* cap_owner(proc_t* proc) {
    return proc_get_proc(proc);
}

bool proc_cap_has(proc_t* proc, uint32_t type, uint32_t rights) {
    proc = cap_owner(proc);
    if(proc == NULL)
        return false;
    if(cnode_has_root(&proc->cnode))
        return true;
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        const cap_t* c = &proc->cnode.slots[i];
        if(c->type == type && cap_rights_ok(c, rights))
            return true;
    }
    return false;
}

bool proc_cap_check_frame(proc_t* proc, uint64_t paddr, uint64_t size, uint32_t rights) {
    proc = cap_owner(proc);
    if(proc == NULL)
        return false;
    if(cnode_has_root(&proc->cnode))
        return true;
    uint64_t end = paddr + size;
    if(end < paddr) /* overflow: an empty/wrapped range never matches */
        return false;
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        const cap_t* c = &proc->cnode.slots[i];
        if(c->type != CAP_FRAME || !cap_rights_ok(c, rights))
            continue;
        uint64_t cb = c->obj.frame.paddr;
        uint64_t ce = cb + c->obj.frame.size;
        if(ce < cb)
            ce = ~0ull; /* wrapped cap range extends to the top of the space */
        if(paddr >= cb && end <= ce)
            return true;
    }
    return false;
}

bool proc_cap_check_irq(proc_t* proc, uint32_t irq, uint32_t rights) {
    proc = cap_owner(proc);
    if(proc == NULL)
        return false;
    if(cnode_has_root(&proc->cnode))
        return true;
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        const cap_t* c = &proc->cnode.slots[i];
        if(c->type == CAP_IRQ && c->obj.irq.irq == irq && cap_rights_ok(c, rights))
            return true;
    }
    return false;
}

/*
 * Match a process-flavoured cap (CAP_PROCESS / CAP_EP / CAP_AS) against a
 * live target pid. The stored uuid must match the target's current uuid so
 * a cap issued against a long-dead pid slot cannot authorize operations on
 * an unrelated new occupant of that slot.
 */
static bool cnode_check_pid(cnode_t* cnode, uint32_t type, int32_t target_pid, uint32_t rights) {
    proc_t* target = proc_get(target_pid);
    if(target == NULL)
        return false;
    for(uint32_t i = 0; i < CNODE_SLOTS; i++) {
        const cap_t* c = &cnode->slots[i];
        if(c->type != type || !cap_rights_ok(c, rights))
            continue;
        const cap_proc_obj_t* o = &c->obj.proc; /* shared {pid, uuid} layout */
        if(o->pid == target_pid && o->uuid == target->info.uuid)
            return true;
    }
    return false;
}

bool proc_cap_check_proc(proc_t* proc, int32_t target_pid, uint32_t rights) {
    proc = cap_owner(proc);
    if(proc == NULL)
        return false;
    if(cnode_has_root(&proc->cnode))
        return true;
    return cnode_check_pid(&proc->cnode, CAP_PROCESS, target_pid, rights);
}

bool proc_cap_check_ep(proc_t* proc, int32_t serv_pid, uint32_t rights) {
    proc = cap_owner(proc);
    if(proc == NULL)
        return false;
    if(cnode_has_root(&proc->cnode))
        return true;
    return cnode_check_pid(&proc->cnode, CAP_EP, serv_pid, rights);
}

/* ======================================================================
 * Exec-time capability policy.
 *
 * Rules are keyed by an executable path and installed from userland
 * (SYS_CAP_POLICY_ADD). proc_cap_on_exec() applies them right after the
 * uid-based cnode reset: the first whitespace-separated token of the new
 * procinfo.cmd is compared against each rule, and every matching rule's
 * caps are built straight into the fresh cnode. This makes the policy
 * independent of process-start order - a service started before, during or
 * after the policy install (and every restart of it) gets exactly the caps
 * its rule declares.
 * ====================================================================== */

#define CAP_POLICY_RULE_MAX   32
#define CAP_POLICY_ENTRY_MAX  16

typedef struct {
    uint32_t type;
    uint32_t rights;
    uint64_t a;
    uint64_t b;
} cap_policy_entry_t;

typedef struct {
    char               cmd[CAP_POLICY_CMD_MAX];
    uint32_t           num;
    cap_policy_entry_t entries[CAP_POLICY_ENTRY_MAX];
} cap_policy_rule_t;

static cap_policy_rule_t _cap_policy[CAP_POLICY_RULE_MAX];
static uint32_t _cap_policy_num = 0;

/* Build the cap a policy entry describes, mirroring proc_cap_mint's
   per-type object semantics. Returns false for entries that cannot be
   materialized right now (e.g. a process-flavoured cap whose target
   instance is dead - same refusal rule as mint). */
static bool cap_policy_build_cap(const cap_policy_entry_t* e, cap_t* c) {
    memset(c, 0, sizeof(cap_t));
    c->type = e->type;
    c->rights = e->rights;
    switch(e->type) {
    case CAP_ROOT:
        /* installable via policy on purpose: the policy file is root-owned,
           so this is config-level delegation of the root authority to a
           trusted boundary image (e.g. /bin/login needing setuid) */
        return true;
    case CAP_FRAME:
        c->obj.frame.paddr = e->a;
        c->obj.frame.size = e->b;
        return true;
    case CAP_IRQ:
        c->obj.irq.irq = (uint32_t)e->a;
        return true;
    case CAP_DMA:
        c->obj.dma.block = (int32_t)e->a;
        return true;
    case CAP_EP:
    case CAP_PROCESS:
    case CAP_AS: {
        /* pin the target's current uuid so the cap dies with that instance */
        proc_t* t = proc_get((int32_t)e->a);
        if(t == NULL)
            return false;
        c->obj.proc.pid = (int32_t)e->a;
        c->obj.proc.uuid = t->info.uuid;
        return true;
    }
    default:
        return false;
    }
}

static void cap_policy_on_exec(proc_t* proc) {
    const char* cmd = proc->info.cmd;
    if(cmd[0] == 0)
        return;
    for(uint32_t r = 0; r < _cap_policy_num; r++) {
        cap_policy_rule_t* rule = &_cap_policy[r];
        uint32_t len = strlen(rule->cmd);
        if(len == 0 || strncmp(cmd, rule->cmd, len) != 0)
            continue;
        char tail = cmd[len]; /* first-token match: NUL or space must follow */
        if(tail != 0 && tail != ' ')
            continue;
        for(uint32_t e = 0; e < rule->num; e++) {
            cap_t c;
            if(cap_policy_build_cap(&rule->entries[e], &c))
                cap_cnode_insert(&proc->cnode, &c);
        }
    }
}

void proc_cap_grant_root(proc_t* proc) {
    if(proc == NULL)
        return;
    cap_cnode_init(&proc->cnode);
    cap_t root;
    memset(&root, 0, sizeof(cap_t));
    root.type = CAP_ROOT;
    root.rights = CAP_R | CAP_W | CAP_X | CAP_GRANT;
    cap_cnode_insert(&proc->cnode, &root);
}

void proc_cap_on_exec(proc_t* proc) {
    if(proc == NULL)
        return;
    if(proc->info.uid <= 0)
        proc_cap_grant_root(proc);
    else
        cap_cnode_init(&proc->cnode);
    /* exec-time policy rides on top of the uid-based reset: a de-rooted
       (uid > 0) image ends up holding exactly its rule's caps */
    cap_policy_on_exec(proc);
}

void proc_cap_on_setuid(proc_t* proc, int32_t new_uid) {
    if(proc == NULL)
        return;
    if(new_uid > 0)
        cap_cnode_init(&proc->cnode);
}

/* ======================================================================
 * Syscall-level entry points (SYS_CAP_* implementations).
 * ====================================================================== */

/*
 * SYS_CAP_MINT: create an object cap in the caller's own cnode. CAP_ROOT
 * only - minting is how the root authority manufactures delegable caps;
 * CAP_ROOT itself can never be minted. Returns the new slot or -1.
 */
int32_t proc_cap_mint(uint32_t type, uint32_t rights, cap_mint_arg_t* uarg) {
    proc_t* cproc = cap_owner(get_current_proc());
    if(cproc == NULL || !cnode_has_root(&cproc->cnode))
        return -1;

    cap_t c;
    memset(&c, 0, sizeof(cap_t));
    c.type = type;
    c.rights = rights & (CAP_R | CAP_W | CAP_X | CAP_GRANT);

    uint64_t a = 0;
    uint64_t b = 0;
    if(uarg != NULL) {
        /* the object args live in user memory: never deref a pointer that
           could name kernel VA (B1 / isolation invariant) */
        if(!user_ptr_ok(cproc, (ewokos_addr_t)uarg, sizeof(*uarg)))
            return -1;
        a = uarg->a;
        b = uarg->b;
    }

    switch(type) {
    case CAP_FRAME:
        c.obj.frame.paddr = a;
        c.obj.frame.size = b;
        break;
    case CAP_IRQ:
        c.obj.irq.irq = (uint32_t)a;
        break;
    case CAP_EP:
    case CAP_PROCESS:
    case CAP_AS: {
        /* pin the target's current uuid so the cap dies with that instance */
        proc_t* t = proc_get((int32_t)a);
        if(t == NULL)
            return -1;
        c.obj.proc.pid = (int32_t)a;
        c.obj.proc.uuid = t->info.uuid;
        break;
    }
    case CAP_DMA:
        c.obj.dma.block = (int32_t)a;
        break;
    default: /* CAP_ROOT and CAP_INVALID are not mintable */
        return -1;
    }
    return cap_cnode_insert(&cproc->cnode, &c);
}

/*
 * SYS_CAP_GRANT: forward one of the caller's caps to another proc's cnode.
 * The source cap must carry CAP_GRANT (CAP_ROOT holders may forward
 * anything), and the forwarded rights are attenuated by rights_mask
 * (0 = forward the source rights verbatim, still bounded by them).
 * Returns the target's slot or -1.
 */
int32_t proc_cap_grant(int32_t target_pid, uint32_t src_slot, uint32_t rights_mask) {
    proc_t* cproc = cap_owner(get_current_proc());
    proc_t* target = cap_owner(proc_get(target_pid));
    if(cproc == NULL || target == NULL || src_slot >= CNODE_SLOTS)
        return -1;

    const cap_t* src = &cproc->cnode.slots[src_slot];
    if(src->type == CAP_INVALID)
        return -1;

    bool root = cnode_has_root(&cproc->cnode);
    if(!root && (src->rights & CAP_GRANT) == 0)
        return -1;

    cap_t c = *src;
    c.rights &= (rights_mask == 0) ? ~0U : rights_mask;
    if(c.rights == 0)
        return -1;
    return cap_cnode_insert(&target->cnode, &c);
}

/* SYS_CAP_REVOKE: drop a cap from the caller's own cnode. */
int32_t proc_cap_revoke(uint32_t slot) {
    proc_t* cproc = cap_owner(get_current_proc());
    if(cproc == NULL || slot >= CNODE_SLOTS)
        return -1;
    if(cproc->cnode.slots[slot].type == CAP_INVALID)
        return -1;
    cap_cnode_revoke(&cproc->cnode, slot);
    return 0;
}

/*
 * SYS_CAP_GET: read back one of the caller's own caps for introspection.
 * slot < 0 queries nothing and only returns the caller's sid (the identity
 * a peer would validate a grant against).
 */
int32_t proc_cap_get(int32_t slot, cap_info_t* uinfo) {
    proc_t* cproc = cap_owner(get_current_proc());
    if(cproc == NULL || uinfo == NULL)
        return -1;
    /* uinfo is written back into user memory: keep it out of kernel VA (B1) */
    if(!user_ptr_ok(cproc, (ewokos_addr_t)uinfo, sizeof(*uinfo)))
        return -1;
    memset(uinfo, 0, sizeof(cap_info_t));
    uinfo->sid = cproc->sid;
    if(slot < 0)
        return 0;
    if(slot >= CNODE_SLOTS)
        return -1;
    uinfo->cap = cproc->cnode.slots[slot];
    return 0;
}

/*
 * SYS_CAP_POLICY_ADD: install one entry into the exec-time policy. CAP_ROOT
 * only (the caller is /sbin/init after parsing /etc/cap.json). Entries
 * sharing a cmd merge into that cmd's rule; the caps take effect at every
 * later exec of a matching image. Returns 0 or -1.
 */
int32_t proc_cap_policy_add(const cap_policy_add_t* uarg) {
    proc_t* cproc = cap_owner(get_current_proc());
    if(cproc == NULL || !cnode_has_root(&cproc->cnode) || uarg == NULL)
        return -1;
    /* uarg (incl. its cmd[256]) is read from user memory: validate first (B1) */
    if(!user_ptr_ok(cproc, (ewokos_addr_t)uarg, sizeof(*uarg)))
        return -1;
    if(uarg->cmd[0] == 0 || uarg->type == CAP_INVALID || uarg->type > CAP_DMA)
        return -1; /* CAP_INVALID/unknown types are not installable */

    cap_policy_rule_t* rule = NULL;
    for(uint32_t i = 0; i < _cap_policy_num; i++) {
        if(strncmp(_cap_policy[i].cmd, uarg->cmd, CAP_POLICY_CMD_MAX) == 0) {
            rule = &_cap_policy[i];
            break;
        }
    }
    if(rule == NULL) {
        if(_cap_policy_num >= CAP_POLICY_RULE_MAX)
            return -1;
        rule = &_cap_policy[_cap_policy_num++];
        memset(rule, 0, sizeof(cap_policy_rule_t));
        sstrncpy(rule->cmd, uarg->cmd, CAP_POLICY_CMD_MAX - 1);
    }
    if(rule->num >= CAP_POLICY_ENTRY_MAX)
        return -1;

    cap_policy_entry_t* e = &rule->entries[rule->num++];
    e->type = uarg->type;
    e->rights = uarg->rights & (CAP_R | CAP_W | CAP_X | CAP_GRANT);
    e->a = uarg->a;
    e->b = uarg->b;
    return 0;
}
