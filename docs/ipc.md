# IPC and Multitasking Design in EwokOS

This document describes the **inter-process communication (IPC)** mechanism
of EwokOS and the **multitasking design** it is built on: the task model,
the scheduler, the single-task "context hijack" server mode, and the
multi-task (`IPC_MULTI_TASK`) worker-thread-pool server mode.

## 1. Architecture Overview

EwokOS IPC is a **synchronous remote-procedure-call** mechanism. A client
process calls a named command of a server process; the kernel delivers the
request, runs the server's handler, and delivers the reply back. No ports,
no pipes, no message queues exist in the kernel — the kernel itself
*is* the message broker.

```
┌─────────────────────┐                        ┌──────────────────────────────┐
│ client process      │                        │ server process               │
│                     │  SYS_IPC_CALL          │                              │
│  ipc_call(pid,cmd,  │ ─────────────────────► │  ipc entry (user function)   │
│        in, out)     │     kernel allocates   │    ipc_get_arg(uid)          │
│                     │     an ipc task (uid)  │    ... handle cmd ...        │
│  blocks until       │                        │    ipc_set_return(uid, out)  │
│  reply arrives      │ ◄───────────────────── │    ipc_end()                 │
│  ipc_get_return()   │  SYS_IPC_SET_RETURN +  │                              │
│                     │  SYS_IPC_END wake      │                              │
└─────────────────────┘                        └──────────────────────────────┘
```

Key components:

| Component | Path | Role |
|---|---|---|
| kernel ipc core | `kernel/kernel/src/ipc.c`, `kernel/kernel/include/kernel/ipc.h` | task slots, wait queues, worker pool, syscall implementations |
| task model / scheduler | `kernel/kernel/src/proc.c`, `kernel/kernel/src/schedule.c` | `proc_t`, ready queues, context switch, block/wake |
| syscall dispatcher | `kernel/kernel/src/svc.c` | thin dispatcher into `proc_ipc_*` |
| syscall numbers | `kernel/kernel/include/syscalls.h` | `SYS_IPC_SETUP/CALL/...` |
| flags / states | `kernel/kernel/include/procinfo.h` | `IPC_MULTI_TASK`, `IPC_NON_RETURN`, task states |
| client/server library | `system/basic/libc/libewoksys/ewoksys/src/ipc.c`, `ipc_serv.c` | `ipc_call()`, `ipc_serv_run()`, `ipc_serv_reg()` |
| payload marshalling | `system/basic/libc/libewoksys/ewoksys/src/proto.c` (`proto_t`) | length-prefixed byte packages |
| name service | `system/basic/sys/core/core.c` | registers `ipc_serv_id` → pid |
| bulk data path | `kernel/kernel/src/mm/shm.c` | shared memory for payloads too large to copy (see `docs/g2d.md`) |

## 2. Multitasking Foundation

IPC is implemented directly on top of the kernel task model, so the
multitasking design is described first.

### 2.1 Task model

Every schedulable entity is a `proc_t` (defined in
`kernel/kernel/include/kernel/proc.h`) living in a fixed-size task table
(`max_task_num` from `kernel.conf`). There are three task types
(`procinfo.h`):

| Type | Meaning |
|---|---|
| `TASK_TYPE_PROC` | a process: owns its address space (`proc_space_t`) |
| `TASK_TYPE_THREAD` | a thread: shares the parent process's `proc_space_t` (same page tables, heap, shm table, ipc server), has its own context and its own kernel-allocated user stack (`THREAD_STACK_PAGES` = 64 pages) |
| `TASK_TYPE_VFORK` | a fork variant used by `exec` chains |

Each task carries:

- `info` (`procinfo_t`): `pid`, `uuid` (monotonic, guards pid-slot reuse),
  `father_pid`, `type`, `core` (pinned core), `state`, `priority`, `cmd`;
- `ctx` (`context_t`): the full saved register set (pc/lr/sp/gprs);
- `space`: pointer to the owning `proc_space_t` (page directory, heap,
  shm table, **ipc_server**, signal/interrupt state, thread stacks);
- `ipc_res`: the client-side reply slot (one in-flight reply per task);
- `ipc_wait_item` / `ipc_waiting_on`: embedded wait-queue node used when
  the task blocks on a busy IPC server;
- `ipc_task`: in multi-task mode, the request this worker thread serves;
- `block_by` / `wake_by` / `wake_pending`: node-scoped block/wake tokens
  with sticky-wake latching, so a wake for one VFS node never releases a
  task blocked on another node.

Task lifecycle states (`procinfo.h`):

```
UNUSED → CREATED → READY ⇄ RUNNING → ZOMBIE → (funeral) → UNUSED
                    │
                    ├─ SLEEPING  (timed sleep, SYS_USLEEP)
                    ├─ WAIT      (waitpid / idle proc parking)
                    └─ BLOCK     (ipc wait, semaphore, vfs wait, parked worker)
```

Zombies are buried by a 1 Hz funeral sweep (`proc_zombie_funeral`), which
frees page tables, stacks and task-table slots.

### 2.2 Address spaces

A process owns one `proc_space_t`: page directory (`vm`), user heap
(`malloc_base` / `rw_heap_base`), up to `SHM_MAX` (128) mapped shared
memory segments, the IPC server state and the thread-stack pool. Threads
created by the same process point at the *same* `proc_space_t`, so they
share code, data and heap with zero synchronization overhead from the
kernel's view; concurrency control between them is user-space's job
(mutexes/rwlocks from `libewoksys` pthread, backed by kernel binary
semaphores).

Context switching swaps the translation table base only when moving
between tasks with different spaces (`proc_switch`, `proc.c`).

### 2.3 Scheduler

- **Per-core ready queues**: `_ready_queue[CPU_MAX_CORES]` FIFO queues;
  each task is pinned to one core (`info.core`, assigned at creation).
  There is no cross-core load balancing — SMP is used by pinning servers
  and their clients appropriately.
- `schedule(ctx)` pops the head of the current core's queue
  (`proc_get_next_ready`, skipping stale non-READY entries); if the queue
  is empty the per-core idle proc runs (`WAIT` state until needed).
- **Priority aging**: `info.priority` seeds `priority_count`; the timer
  tick (`renew_kernel_tic` → `renew_priority_counter`) decrements it for
  every RUNNING/READY task. When a READY task's counter reaches zero it is
  re-queued (round-robin within the queue); the RUNNING task's counter is
  reloaded. Preemption points are syscall exit, interrupts and explicit
  `SYS_YIELD`.
- `proc_switch(ctx, to, quick)` performs the actual switch: saves the
  outgoing context, loads `to->ctx`, re-enqueues the outgoing task if it
  is still RUNNING (`quick` = push to queue head), and — critically for
  IPC — rewrites `to`'s context when the target has a pending interrupt,
  signal or IPC request to serve (see §4).
- All scheduler/task-table mutations run under the global `proc_lock`
  (`proc_lock_enter/leave`); syscalls are additionally serialized by
  `kernel_lock`, so IPC state machines only need fine-grained per-server
  spinlocks (`mcore_lock`) on SMP builds.

### 2.4 Block / wake

`proc_block(ctx, proc)` takes the current task out of the scheduler
(state BLOCK). `proc_wakeup(proc)` moves it back to READY. VFS-style
paths use the token variants `proc_block_by` / `proc_wakeup_by`; a wake
arriving before the block is latched (`wake_pending` + `wake_by`) and
consumed by the matching block, preventing both lost and spurious wakes.

## 3. IPC Basics

### 3.1 Syscall surface

| Syscall | Direction | Purpose |
|---|---|---|
| `SYS_IPC_SETUP` | server | register handler entry, `extra_data`, flags |
| `SYS_IPC_CALL` | client | issue request `(serv_pid, call_id, arg)` → returns ipc `uid` |
| `SYS_IPC_GET_ARG` | server | fetch client pid, `call_id`, argument of the request being served |
| `SYS_IPC_SET_RETURN` | server | deliver reply payload to the client's reply slot |
| `SYS_IPC_GET_RETURN` | client | collect the reply (blocks while pending) |
| `SYS_IPC_END` | server | finish serving the current request |
| `SYS_IPC_DISABLE` / `SYS_IPC_ENABLE` | server | stop/resume accepting new requests |
| `SYS_IPC_PING` / `SYS_IPC_READY` | both | readiness handshake (`ipc_wait_ready`) |

Return codes of `SYS_IPC_CALL`: positive = ipc `uid`;
`IPC_ERROR_RETRY (-1)` = server busy/disabled (the kernel blocks the
client instead of returning where possible), `IPC_ERROR_SELF (-2)` =
self-call, `IPC_ERROR_NO_READY (-3)` = no such server / service not set
up.

### 3.2 Payload format

Arguments and replies are `proto_t` packages: a flat, length-prefixed
byte buffer (`data`, `size`, `total_size`) appended/read item by item
(`proto_add_int/str/...`, `proto_read_int/str/...`). The kernel copies
the package into the ipc task slot with `proto_copy`; if the receiver's
buffer is too small, the syscall returns the *required size* and the
caller retries with a resized package (both `ipc_call` and the server
loop implement this two-step protocol).

Large bulk data is not pushed through the IPC copy path; servers and
clients share memory segments instead (`shmget/shmat`, `SYS_PROC_SHM_*`)
and pass only identifiers through IPC — see `docs/g2d.md` for the
zero-copy graphics pipeline built on this.

### 3.3 Request bookkeeping

A server holds `ipc_server_t` in its `proc_space_t`:

- `tasks[IPC_CTX_MAX]` (`IPC_CTX_MAX = 8`): fixed pool of `ipc_task_t`
  request slots — no kernel heap allocation per request;
- each `ipc_task_t`: global unique `uid`, `state`
  (`IPC_IDLE/BUSY/RETURN`), copied argument (`arg_ret`),
  `client_pid`+`client_uuid` (slot-reuse guard), `client_intr` (client
  called from its interrupt handler), `call_id`, and — multi-task mode —
  `handler_pid`+`handler_uuid` of the worker thread serving it;
- `wait_head/wait_tail`: intrusive doubly-linked wait queue of blocked
  clients (each client contributes its embedded `ipc_wait_item`);
- `lock`: per-server spinlock used on `KERNEL_SMP` builds;
- `entry`, `extra_data`, `flags`, `disabled`, `multi_task`, and the
  single-task-mode machinery (`saved_state`, `saved_ipc_res`, `stack`,
  `do_switch`, `restore_pending`).

The client side keeps exactly one reply slot per task:
`proc_t.ipc_res` (`ipc_res_t`: `uid`, `state`, `data`). If the call was
issued while the client was running its interrupt handler, the reply
goes to `client->space->interrupt.ipc_res` instead (flagged by
`client_intr` at request time), so interrupt-level clients get their
answers without corrupting any main-context reply.

### 3.4 Naming and registration

Servers are addressed by pid. Well-known services register a name with
the **core process** (`system/basic/sys/core/core.c`):

```c
ipc_serv_reg("ipc_serv.vfs");             // CORE_CMD_IPC_SERV_REG
int pid = ipc_serv_get("ipc_serv.vfs");   // CORE_CMD_IPC_SERV_GET
ipc_call(pid, cmd, &in, &out);
```

A typical server startup (e.g. `system/basic/sys/vfsd/vfsd.c`) is:
register name → `ipc_serv_run(handle, handled, p, flags)` → main loop.
`ipc_serv_run` issues `SYS_IPC_SETUP` with the generic dispatcher
`handle_ipc` as entry, then `SYS_IPC_READY`.

### 3.5 Call flow (client library)

`ipc_call(to_pid, call_id, ipkg, opkg)` in
`system/basic/libc/libewoksys/ewoksys/src/ipc.c`:

1. if `opkg == NULL`, set `IPC_NON_RETURN` in `call_id` (fire and
   forget);
2. `SYS_IPC_CALL` in a retry loop (the kernel itself blocks the client
   on `IPC_ERROR_RETRY` whenever possible, so spinning here only covers
   races);
3. for calls with a reply: `SYS_IPC_GET_RETURN` loop — `-1` means
   "reply not ready yet, the kernel blocked us and woke us", a positive
   value means "buffer too small, resize and retry once".

Server side (`ipc_serv.c::handle_ipc`): `SYS_IPC_GET_ARG` (with the same
resize protocol) → user callback `_ipc_serv_handle(pid, cmd, in, out, p)`
→ `SYS_IPC_SET_RETURN` (unless `IPC_NON_RETURN`) → `SYS_IPC_END`.

`call_id` encoding (`procinfo.h`): bits `0..28` are the user command
(`IPC_NON_RETURN_MASK`), bit 30 is `IPC_LAZY`, bit 31 is
`IPC_NON_RETURN`. Server `flags` are `IPC_NON_BLOCK (0x01)` and
`IPC_MULTI_TASK (0x02)`.

## 4. Single-Task Server Mode (default)

Without `IPC_MULTI_TASK` the server's **main execution context is
hijacked** to run the handler — there is exactly one execution context
serving requests, strictly one at a time, FIFO.

### 4.1 Request arrival

`SYS_IPC_CALL` → `proc_ipc_call()`:

1. resolve server by pid, reject self-calls and missing/disabled
   services (disabled → block the client on the server wait queue);
2. if the server is currently running an interrupt handler, block the
   client until the interrupt finishes;
3. `proc_ipc_req()` takes the FIFO ring tail slot, stamps a fresh global
   `uid`, copies the argument, records the client (pid+uuid, and whether
   it called from interrupt context);
4. the client's reply slot goes `IPC_BUSY` and the client gets the `uid`;
5. if this request is the new queue head, `proc_ipc_do_task()`
   - snapshots the server's scheduler state
     (`proc_save_state`: state, sleep counter, in-flight `ipc_res`),
   - sets `ipc_server.do_switch = true`,
   - switches to the server immediately (unless the call carries
     `IPC_LAZY`, in which case the switch is deferred until the server
     is scheduled anyway).

### 4.2 The context hijack

When `proc_switch()` is about to resume a `TASK_TYPE_PROC` target whose
`ipc_server.do_switch` is set, it rewrites the target context instead of
restoring it:

- saves the server's real context into `ipc_server.saved_state.ctx`;
- `pc = lr = ipc_server.entry`, `gpr[0] = ipc uid`,
  `gpr[1] = extra_data`;
- `sp` = a dedicated lazily-allocated IPC stack
  (`ipc_server.stack`, `THREAD_STACK_PAGES` pages) — the handler runs on
  its own stack so the server's main-loop stack is untouched.

The server then runs the user handler from the entry point, exactly like
a function call whose only job is to end with `SYS_IPC_END`.
`proc_ipc_sync_serving()` marks such a context: it must not enter real
sleeps/blocks, because the saved-state restore machine would be
stranded.

### 4.3 Completion: `SYS_IPC_END`

`proc_ipc_end()` (single-task path):

1. `proc_restore_state()` puts the server's saved scheduler state and
   context back (including its in-flight client-side `ipc_res`, so a
   server that is *itself* a client of another server survives the
   hijack);
2. if the server was BLOCK/SLEEPING when the request arrived it is made
   READY so its main loop continues;
3. the request slot is freed (`proc_ipc_close`, FIFO head pop) and queued
   clients are admitted (`proc_ipc_wakeup`);
4. if the call expected a reply (`!IPC_NON_RETURN`) the client is woken;
5. if another request is already queued, the server immediately serves it
   (save → hijack → next); otherwise `schedule()` runs.

`IPC_NON_BLOCK` (throughput mode, set by servers with their own main
loop such as `core`, `xwm`, `splashd`) changes step 5: the kernel does
not synchronously switch to the just-woken client when the queue is
empty, preferring to drain queued requests instead.

### 4.4 Flow control

- A server accepts at most `IPC_CTX_MAX = 8` queued requests; beyond
  that (or while disabled) the client is **blocked inside the kernel**
  on the server's wait queue (`proc_ipc_wait`) — never returned a busy
  error that userspace would spin on.
- On every completion, `proc_ipc_wakeup()` admits at most
  `min(free_slots, IPC_WAKE_BATCH_LIMIT = 2)` waiters per edge,
  skipping stale entries (dead or already-released waiters) so a single
  bad entry cannot strand the queue.
- `SYS_IPC_DISABLE` only succeeds when no request is in flight;
  `SYS_IPC_ENABLE` wakes queued clients.

## 5. Multi-Task Server Mode (`IPC_MULTI_TASK`)

Setting `IPC_MULTI_TASK` in `SYS_IPC_SETUP` flags (e.g.
`device_run(&dev, "/", FS_TYPE_DIR, 0777, true)` in
`system/basic/sys/rootfs/sdfsd.c`) switches the server to fully
**concurrent request handling**:

- the server's main context is **never hijacked** — requests run in
  kernel-spawned **worker threads inside the server process** (sharing
  its address space), so the server can keep running its own main loop;
- requests complete **out of order**; each is tracked by its `uid`;
- no interrupt-state gate: a worker is an independent context and may
  run while the server's main context handles an interrupt.

### 5.1 Persistent worker pool

Instead of creating and terminating a thread per request, the server
keeps a **persistent pool** of parked worker threads
(`ipc_server_t.pool[IPC_TASK_POOL_MAX_NUM]`):

| Constant | Value | Meaning |
|---|---|---|
| `IPC_TASK_POOL_MIN_NUM` | 2 | base workers; always kept alive |
| `IPC_TASK_POOL_MAX_NUM` | 16 | hard pool size limit |
| `IPC_TASK_SELF_QUIT_TIMEOUT` | 3 s | idle time before an *extra* worker quits |

Lifecycle:

1. **Spawn parked** — `proc_ipc_pool_spawn()` uses `kfork_raw` to create a
   `TASK_TYPE_THREAD` inside the server proc: its context is prepared
   (`pc = lr = entry`, `gpr[1] = extra_data`, own thread stack) but it
   stays in BLOCK. `ipc_pool_replenish()` keeps at least MIN_NUM live
   members; it runs on every `SYS_IPC_CALL` iteration and on pool waits,
   retrying after earlier resource exhaustion (task table or thread
   stack slots).
2. **Assign** — `ipc_pool_assign()` scans for a parked member
   (BLOCK, no task attached, not quit-marked) and *binds* the request:
   under the server lock it rewrites the worker's saved context
   (`gpr[0] = uid`, fresh `sp` on the worker's own stack), records
   `worker->ipc_task = ipc` / `ipc->handler_pid+uuid`, then wakes the
   worker. The scheduler later restores exactly this context.
3. **Grow on demand** — if no member is idle and the pool is below
   MAX_NUM, one new parked worker is spawned, registered and bound in
   the same call. Only when the pool is **at MAX_NUM and every member
   is busy** does the client block inside the kernel
   (`proc_ipc_wait_pool`) until a worker parks; the task slot acquired
   for that attempt is released first so nothing is wasted while
   blocked.
4. **Park, don't exit** — on `SYS_IPC_END` a pool member does not
   terminate. `proc_ipc_pool_park()` flips it RUNNING→BLOCK under the
   server+proc locks *before* issuing any wakeups (flip-before-wake
   guarantees a retrying client either sees the worker idle or is queued
   and woken — never stranded), stamps `idle_sec`, and sleeps until the
   next assignment rewrites its context.
5. **Idle shrink** — the 1 Hz `renew_kernel_sec` tick calls
   `proc_ipc_pool_shrink()` for every multi-task server: if more than
   MIN_NUM members are alive, ONE member parked idle for at least
   `IPC_TASK_SELF_QUIT_TIMEOUT` seconds is quit-marked and woken; the
   worker notices the mark right after waking, clears its pool slot and
   exits (the funeral frees its stack and proc slot). One member per
   tick keeps the shrink gradual; base members are never targeted.

Pool slots store `pid`+`uuid`; resolution (`ipc_pool_slot_worker`) drops
stale slots (dead worker or reused pid slot) so replenish can refill
them.

### 5.2 Request lifecycle in multi-task mode

```
client                kernel                        worker thread
  │ SYS_IPC_CALL        │                               │
  ├────────────────────►│ proc_ipc_req: free slot,      │
  │                     │   uid, copy arg               │
  │                     │ ipc_pool_assign:              │
  │                     │   bind (rewrite ctx) + wake   │
  │  (client stays      ├──────────────────────────────►│ runs ipc entry:
  │   blocked in        │                               │   GET_ARG(uid)
  │   GET_RETURN)       │                               │   ... work ...
  │                     │  SYS_IPC_SET_RETURN ◄─────────┤   SET_RETURN(uid,out)
  │ ipc_res = RETURN ◄──┤  (copy into client slot)      │
  │                     │  SYS_IPC_END ◄────────────────┤
  │ wake                │ proc_ipc_finish_task:         │ pool member: PARK
  │◄────────────────────┤   free slot under server      │   (or exit if not
  │                     │   lock, wake client + waiters │    registered)
  │ SYS_IPC_GET_RETURN  │                               │
  ├────────────────────►│ copy reply, clear slot        │
```

Notable details:

- `proc_ipc_call` loops inside the kernel: on task-slot exhaustion it
  blocks the client on the server wait queue, on worker exhaustion on
  the pool wait queue, re-validating the server (pid+uuid) after every
  wake because the server may have exited meanwhile;
- completion goes through `proc_ipc_finish_task()`, which claims the
  task and resolves its client **atomically under the server lock**, so
  a concurrent watchdog abort (timer context, possibly another core) can
  never leave either path operating on a freed/reused slot;
- `SYS_IPC_GET_RETURN` looks the task up **by uid**
  (`proc_ipc_find_task`) since the FIFO head no longer identifies a
  specific request;
- `proc_ipc_serving_task()` validates every server-side syscall
  (GET_ARG / SET_RETURN / END): the executing context must actually be
  the worker bound to that uid — a stale or aborted worker sees "no
  task" and its syscalls become no-ops.

### 5.3 Robustness

- **Watchdog**: servers with in-flight requests sit on
  `_ipc_timeout_queue`. The timer tick accumulates `ipc->counter` only
  while the serving context is continuously runnable — a handler blocked
  on VFS or waiting in an interrupt is legitimate waiting, not a stall.
  After `IPC_TIMEOUT_USEC` (10 s) of continuous runnable stall:
  - multi-task: `proc_ipc_task_abort()` detaches the worker, clears the
    client's reply slot (the client's call fails instead of hanging),
    frees the slot and wakes the client;
  - single-task: the kernel forcibly restores the server's saved state
    (`proc_ipc_timeout`), fails the client and admits waiters; on SMP it
    IPIs the server's core when the restore targets a context running
    elsewhere.
- **pid-slot reuse guards**: every reference (client, worker, pool
  member, waiter) is validated by `uuid` in addition to `pid`.
- **Abort races**: a timed-out worker that later reaches
  SET_RETURN/END finds `ipc_task == NULL` (detached by the abort) and
  just parks or exits.
- **Server death**: `proc_ipc_clear()` drops all in-flight tasks;
  waiters re-resolve the server by pid+uuid on wake and get
  `IPC_ERROR_NO_READY`.

## 6. SMP Considerations

- Syscalls are serialized by `kernel_lock`; task-table/scheduler state
  by `proc_lock`; per-server IPC state (task slots, pool, wait queue)
  additionally by the server spinlock under `KERNEL_SMP`. This layering
  is what lets the watchdog (timer context, any core), `SYS_IPC_END`
  (worker, its core) and `SYS_IPC_CALL` (client, its core) all touch
  the same `ipc_server_t` safely.
- Every task runs pinned to `info.core` on per-core ready queues;
  `proc_switch_multi_core` hands a task to a specific core's queue.
  IPC hand-offs cross cores this way (client on core 0 → server on
  core 1), and timeout recovery sends an IPI when a restore must be
  observed by another core.

## 7. Quick Reference

### Constants

| Constant | Value | Defined in |
|---|---|---|
| `IPC_CTX_MAX` | 8 | `kernel/kernel/include/kernel/ipc.h` |
| `IPC_TASK_POOL_MIN_NUM` / `MAX_NUM` | 2 / 16 | same |
| `IPC_TASK_SELF_QUIT_TIMEOUT` | 3 s | same |
| `IPC_TIMEOUT_USEC` | 10 s | `kernel/kernel/include/kernel/kernel.h` |
| `IPC_WAKE_BATCH_LIMIT` | 2 | `kernel/kernel/src/ipc.c` |
| `THREAD_STACK_PAGES` | 64 | `kernel/kernel/include/kernel/proc.h` |
| `IPC_NON_BLOCK` / `IPC_MULTI_TASK` | 0x01 / 0x02 | `kernel/kernel/include/procinfo.h` |
| `IPC_NON_RETURN` / `IPC_LAZY` | bit 31 / bit 30 of `call_id` | same |

### Server-mode comparison

| Aspect | single-task (default) | multi-task (`IPC_MULTI_TASK`) |
|---|---|---|
| handler execution context | server main context (hijacked) | dedicated worker thread per request |
| concurrency | 1 request at a time, FIFO | up to 8 in-flight, out of order |
| server main loop | suspended while serving | untouched |
| handler stack | dedicated `ipc_server.stack` | each worker's own thread stack |
| completion order | FIFO (`ipc_end` pops head) | by uid (`proc_ipc_find_task`) |
| blocking handler | illegal (`proc_ipc_sync_serving`) | fine (only the worker blocks) |
| from interrupt-context client | supported via `interrupt.ipc_res` | same |
| extra machinery | save/restore state machine | worker pool: park/assign/shrink |

### Source map

| File | Content |
|---|---|
| `kernel/kernel/src/ipc.c` | wait queues, task slots, pool, `SYS_IPC_*` implementations |
| `kernel/kernel/src/proc.c` | task table, `kfork`, `proc_switch` (hijack point), scheduler queues, watchdog ticks, `proc_ipc_pool_spawn/park` |
| `kernel/kernel/src/schedule.c` | `schedule()` |
| `kernel/kernel/src/svc.c` | syscall dispatch |
| `system/basic/libc/libewoksys/ewoksys/src/ipc.c` | `ipc_call` / `ipc_wait_ready` / disable-enable |
| `system/basic/libc/libewoksys/ewoksys/src/ipc_serv.c` | `ipc_serv_run` / `ipc_serv_reg` / generic handler |
| `system/basic/sys/core/core.c` | name service for ipc servers |
| `system/basic/sys/rootfs/sdfsd.c` | example multi-task server (rwlock-protected concurrent FS) |
