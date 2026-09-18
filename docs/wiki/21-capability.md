# 21 The Capability Permission Mechanism

> Language: **English** | [中文](21-capability.zh.md)
>
> Goal: understand EwokOS's capability permission model — on what grounds
> may a process map device memory, register an interrupt, or send signals
> to others? Read the cnode capability table, the kernel's check points,
> the mint/grant delegation mechanism, and how the declarative policy in
> `/etc/cap.json` lands "least privilege" onto every driver.
> Corresponding sources: [cap.h](../../kernel/kernel/include/cap.h),
> [cap.c](../../kernel/kernel/src/cap.c),
> [cap_policy.c](../../system/basic/sys/init/cap_policy.c).
> Terminology (capability/cnode/mint/grant…) is in the
> [glossary](99-glossary.md).

## 21.1 Why Capabilities: Starting from root Takes All

Ch. 08's food-for-thought asked: `SYS_MEM_MAP` (mapping device memory)
should only be given to trusted driver processes — how should the kernel
tell them apart? The answer back then was to check `proc->info.uid` — uid 0
(root) gets through.

That answer works, but it's too coarse:

- **A one-track switch**: uid only has two states, "root or not". The UART
  driver and the disk driver are both root; if the former has a
  vulnerability, it can still read and write the disk controller's
  registers;
- **Microkernels especially need finer grain**: as Ch. 01 said, EwokOS
  pushes drivers and filesystems into userland. More than half of the
  system's processes are doing "what the kernel used to do" — if they're
  all root, the isolation the microkernel bought exists in name only;
- **The blast radius must be controllable**: the Principle of Least
  Privilege — every component gets only the permissions necessary for its
  job, so when compromised it can at most mess up its own stall.

Capabilities are the solution: **don't ask "who are you", only ask "what
tickets do you hold"**. Each ticket precisely describes "what operation may
be performed on which object", and the kernel checks tickets on every
privileged operation.

## 21.2 What a Capability Is: a Ticket of Object + Rights

A capability is a triple ticket:

```
┌─────────────────────────────────────────┐
│  type    the object type (device memory? interrupt? process?) │
│  rights  the rights bits (read/write/execute/grantable)       │
│  object  the object identifier (stored by value: address, irq no…) │
└─────────────────────────────────────────┘
```

Every process holds a **cnode** (capability node — the capability table),
with at most 64 slots (`CNODE_SLOTS = 64`), one ticket per slot. When a
process wants to do a privileged operation, the kernel rummages through
its cnode: is there a ticket whose **type matches, whose object covers the
target, and whose rights bits are complete**? If yes, pass; if no, refuse.

Compared with traditional Unix:

| | The Unix uid model | The capability model |
|---|---|---|
| Basis of judgment | who you are (uid==0?) | what tickets you hold |
| Granularity | root = all permissions | each ticket: one object, one class of operation |
| Transfer | the setuid bit (a wholesale transformation) | ticket-by-ticket forwarding, only ever getting weaker |
| Revocation | very hard | just tear the ticket |

## 21.3 EwokOS's cap Types and Rights Bits

[cap.h](../../kernel/kernel/include/cap.h) defines 7 ticket types:

| Type | Object guarded | Typical holder |
|---|---|---|
| `CAP_FRAME` | a physical address range (RAM / MMIO) | drivers (mapping device registers) |
| `CAP_IRQ` | an interrupt number | drivers, timerd |
| `CAP_DMA` | a DMA memory block | drivers using `dma_user_alloc()` |
| `CAP_EP` | an IPC endpoint (a service process) | a service's clients |
| `CAP_PROCESS` | a target process | processes needing to signal across users / adjust priorities |
| `CAP_AS` | an address space | (reserved, address-space-level operations) |
| `CAP_ROOT` | the system root authority | kernel processes, login (see §21.9) |

There are only 4 rights bits; their semantics depend on the type:

```c
#define CAP_R       (1U << 0)   /* read    */
#define CAP_W       (1U << 1)   /* write   */
#define CAP_X       (1U << 2)   /* execute */
#define CAP_GRANT   (1U << 3)   /* grantable */
```

- A frame's `W`: allows `SYS_MEM_MAP`ing that physical range into one's own
  address space;
- An irq's `W`: allows registering that interrupt number; `X`: allows
  sending soft interrupts;
- An ep's `X`: allows issuing IPC calls to that service;
- `CAP_GRANT`: allows forwarding this ticket to other processes (§21.6).

## 21.4 Kernel Check Points: Which Syscalls Are Guarded

Ticket checking happens at the syscall entrances
([svc.c](../../kernel/kernel/src/svc.c)). The main check points:

| Syscall | Ticket required | How it's checked |
|---|---|---|
| `SYS_MEM_MAP` (mmio_map) | `CAP_FRAME` + W | the target range must be completely covered by some frame ticket |
| `SYS_INT_SETUP` (register an interrupt) | `CAP_IRQ` + W | interrupt numbers matched one by one |
| `SYS_SOFT_INT` (send a soft interrupt) | `CAP_IRQ` + X | the X bit of any irq ticket suffices |
| `SYS_DMA_*` (DMA alloc/map) | `CAP_DMA` + W | block number matched |
| `SYS_SIGNAL` / adjust priority / cross-user wakeup | `CAP_PROCESS` + W | matched by target pid |
| `SYS_PROC_SET_UID` / `SET_GID` | `CAP_ROOT` | the only channel for switching user identity |
| `SYS_CORE_PROC_READY` | `CAP_ROOT` | only the core process should call it |
| IPC calls (when the service enables `IPC_CAP_CHECK`) | `CAP_EP` + X | matched by the service's pid ([ipc.c](../../kernel/kernel/src/ipc.c)) |

Take `SYS_MEM_MAP`: the check, simplified, is "find a ticket in the cnode
that covers the target":

```c
/* The process wants to map [paddr, paddr+size): walk the cnode,
   looking for a ticket with type==CAP_FRAME, rights containing W,
   and a range that completely covers the target */
proc_cap_check_frame(proc, paddr, size, CAP_W);
```

Note one security detail: **objects are stored by value and re-validated at
check time**. For example, a `CAP_PROCESS` ticket stores "the target pid +
the target's uuid at the time" (pids get reused, uuids don't — see Ch. 07).
When the target process exits and its pid is recycled, the old ticket
naturally goes stale — there's no "the new process the ticket points at
isn't the original one" problem.

## 21.5 The Permission Lifecycle: create / fork / exec / setuid

Where do tickets come from? The kernel defines a clear lifecycle rule for
every process ([proc.c](../../kernel/kernel/src/proc.c) + cap.c):

```
processes created by the kernel (core/vfsd etc., no parent)
        │  granted CAP_ROOT directly — they are the system's "creators"
        ▼
fork    │  copies the parent's cnode as-is — the child inherits all of the parent's tickets
        ▼
exec    │  reset! A new program shouldn't inherit the old program's tickets:
        │    · a system-level image (uid<=0) → re-grant CAP_ROOT
        │    · an ordinary user program (uid>0) → the cnode is emptied
        │  then consult the policy table (§21.7) and place the declared
        │  tickets into the new cnode
        ▼
setuid  │  becoming an ordinary user (uid>0) → all tickets are torn
```

The exec reset is the most critical step in the whole chain: it ensures
**permissions follow "program identity", not "process history"**. The shell
being root doesn't mean every program it launches should be root — the
moment a program execs, it starts from zero, holding only the tickets the
policy declares.

## 21.6 Delegation: mint / grant / revoke

Tickets can also flow between processes, via three syscalls (userland
wrappers in
[cap.c](../../system/basic/libc/libewoksys/ewoksys/src/cap.c)):

```c
int32_t cap_mint(type, rights, a, b);          /* mint an object ticket (CAP_ROOT only) */
int32_t cap_grant(target_pid, src_slot, mask); /* hand a copy of my ticket to someone else */
int32_t cap_revoke(slot);                      /* tear one of my own tickets */
```

Two hard rules:

1. **Minting is centralized**: only CAP_ROOT holders may call `cap_mint`.
   Object tickets (frame/irq/dma…) cannot be conjured out of thin air —
   they must be minted by the root authority;
2. **Forwarding can only attenuate**: the rights bits given by `cap_grant`
   are "original ticket ∩ mask" — you can never give more than you have.
   And what's given is a **copy**: tearing your own ticket doesn't affect
   the one already handed out.

This mechanism is called **delegation**: the root authority needn't do
everything in person — it can deliver "access to a small piece of device
memory" precisely and safely into the hands of the process that needs it.

## 21.7 /etc/cap.json: Declarative Permission Policy

With the mechanism complete, we still lack **policy**: which tickets each
driver should get — written where, handed out by whom?

Two practical constraints decide the design:

- exec resets the cnode → ticket issuance must happen **after** exec;
- drivers are brought up by the shell in init scripts, in an uncontrollable
  order → you can't issue tickets by "scanning already-running processes"
  (they might not have started when you scan).

EwokOS's answer: **pour the policy into the kernel, and let the kernel
issue tickets itself on every exec**.

```
early in /sbin/init's startup:
    read /etc/cap.json (parsed with tinyjson; see cap_policy.c)
    call SYS_CAP_POLICY_ADD entry by entry, pouring it into
        the kernel's policy table
        (32 rules × 16 caps per rule)
        │
thereafter, on every exec (whenever, however many times that process restarts):
    after resetting the cnode, the kernel matches the new program's
    first cmd word against the policy table; on a hit, the declared caps
    are built directly into the new cnode — issuance complete
```

The policy file looks like this (excerpt from
[machine.virt's real config](../../machine.virt/system/etc/basic/cap.json)):

```json
{
    "rules": [
        { "cmd": "/drivers/virt/ttyd", "desc": "UART driver: one PL011 page + irq33", "caps": [
            {"type":"frame", "paddr":"0x09000000", "size":"0x1000", "rights":"rw"},
            {"type":"irq", "irq":33, "rights":"w"}
        ]},

        { "cmd": "/drivers/virt/timerd", "desc": "soft-interrupt service", "caps": [
            {"type":"irq", "irq":0, "rights":"x"}
        ]},

        { "cmd": "/sbin/sdfsd", "desc": "the SD card filesystem", "caps": [
            {"type":"frame", "paddr":"0x0a000000", "size":"0x4000", "rights":"rw"},
            {"type":"irq", "irq":[48,49,50,51,52,53,54,55], "rights":"w"},
            {"type":"dma", "block":0, "rights":"rw"}
        ]},

        { "cmd": "/bin/login", "desc": "the setuid trust boundary", "caps": [
            {"type":"root", "rights":"rwxg"}
        ]}
    ]
}
```

Key points:

- `cmd` matches the **first whitespace-delimited word** of the process's
  `procinfo.cmd` (i.e. the executable path);
- `rights` letters correspond to R/W/X/GRANT;
- a frame's `paddr`/`size` are written as hexadecimal strings (JSON
  integers are only 32-bit);
- irq can be an array, expanding into one ticket per number;
- **`"type":"root"`**: grants CAP_ROOT itself as policy (§21.9).

Two accompanying engineering details:

- **Finer-grained mapping regions**: drivers use
  `mmio_map_offset(offset, size)` to map only the sub-region they need
  (e.g. ttyd maps only the PL011 page; virtio maps only the 0x4000 starting
  at 0x0a000000), so frame tickets can be issued for just that small piece —
  rather than the whole 64MB MMIO window. Mechanism (offset mapping) and
  policy (small-range tickets) enable each other;
- **When there's no cap.json**: init probes with `access()` first; if the
  file doesn't exist, cap setup is skipped entirely and the boot scripts
  run as root the old way — policy is an **optional increment**, not a
  hijacking of existing systems.

With a policy in place, init runs the boot scripts as uid 1 (an ordinary
user), and this JSON is each service's only source of permissions. The boot
log shows every ticket issued:

```
init: cap policy '/etc/cap.json': 11 rule(s)
init: cap /sbin/sdfsd: frame 0xa000000+0x4000 rw
init: cap /sbin/sdfsd: irq 48 w
...
init: cap /bin/login: root rwxg
init: cap policy installed (67 entries)
```

## 21.8 In Practice: Least Privilege for Your Own Driver

Suppose you wrote a new driver `/drivers/virt/myd` needing these resources:
one page of MMIO at `0x09030000`, interrupt 40, and one DMA block.

Three steps:

1. **In the driver code, map only your own turf**:

   ```c
   mmio_map_offset(0x01030000, 0x1000);  /* offset to the page at 0x09030000 */
   interrupt_setup(40, my_irq_entry, 0);
   dma_user_alloc(0);
   ```

2. **Add a rule to cap.json**:

   ```json
   { "cmd": "/drivers/virt/myd", "caps": [
       {"type":"frame", "paddr":"0x09030000", "size":"0x1000", "rights":"rw"},
       {"type":"irq", "irq":40, "rights":"w"},
       {"type":"dma", "block":0, "rights":"rw"}
   ]},
   ```

3. **Reboot and verify**: the boot log should show three lines of `init:
   cap /drivers/virt/myd: ...`, and the driver works. Then do the reverse
   experiment — delete the irq line and boot again: `interrupt_setup` is
   guaranteed to fail. One ticket short, and the job can't be done.

This reverse experiment says it best: **permissions are not
"default-present, blocked on error"; they are "default-absent, present only
when declared"**.

## 21.9 CAP_ROOT and login: the Trust-Boundary Trade-off

Strict capability systems (seL4, EROS, etc.) reject the "one ticket for
everything": even the root task must be explicitly authorized object by
object. EwokOS keeps `CAP_ROOT` as a pragmatic compromise — all
`proc_cap_check_*` checks short-circuit and pass upon seeing CAP_ROOT.

Why keep it?

- The system evolved from the uid model; creator processes like core/vfsd
  touch too many, too scattered things — modeling them one by one yields
  little;
- The compatibility path needs an outlet (old systems without cap.json keep
  working).

But CAP_ROOT's distribution has been narrowed to **one explicit trust
boundary**: `setuid`/`setgid` now requires holding CAP_ROOT (no longer
"uid==0 is enough"), and the only userland program in the whole system that
obtains CAP_ROOT through policy is `/bin/login` — it is the equivalent of
setuid-root in Unix: after authentication passes, it switches to the target
user, and the moment `setuid(uid>0)` lands, all tickets are torn (§21.5);
the new session starts "clean". The permissions a root login gets come from
each subsequent exec re-issuing them per policy — not from login passing
the master ticket down.

This is EwokOS's compromise: **the master ticket exists, but is
institutionally locked in a small box that can be audited**.

## 21.10 Exercises

1. Boot the system in QEMU, count the `init: cap ...` lines in the boot
   log, and verify against `cap.json` how many tickets each rule expands
   into (watch out for irq arrays);
2. Change the ttyd rule's `"rights":"rw"` to `"r"` and reboot: the frame
   check requires the W bit — at which step will ttyd fail?
3. Food for thought: `cap_grant` hands out a copy — why does this design
   make "revoking already-issued permissions" very hard? In connection with
   §21.6, discuss feasible remedies under the cnode model (hint: objects
   validated by value + uuid);
4. Advanced: add a `CAP_PROCESS` policy rule for a tool you wrote under
   `/bin` (in cap_policy.c, the `a` parameter is the target pid — think
   about why tickets whose "objects are only known at runtime" don't fit a
   static cap.json).

## 21.11 Summary

- Capabilities swap "who you are" for "what tickets you have": type +
  rights + object, one 64-slot cnode per process;
- The kernel checks tickets at syscall entrances: frame/irq/dma/ep/process
  each have check points; CAP_ROOT short-circuits through;
- The lifecycle: kernel processes hold the master ticket → fork inherits →
  exec resets and rebuilds per policy → setuid to an ordinary user zeroes
  everything;
- The three delegation operations mint/grant/revoke: minting is
  centralized, forwarding only attenuates, and what's given is a copy;
- `/etc/cap.json` is declarative policy: init pours it into the kernel, and
  the kernel automatically issues tickets by matching cmd on every exec —
  independent of boot order, re-issued automatically on restart;
- login is the only userland program holding CAP_ROOT — the system's setuid
  trust boundary.

Next up is Part 3: the filesystem — how vfsd runs the "everything is a
file" routing; it is itself the service holding the most tickets in this
machine's policy table.
