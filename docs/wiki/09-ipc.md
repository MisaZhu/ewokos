# 09 IPC: Inter-Process Communication

> Language: **English** | [中文](09-ipc.zh.md)
>
> Goal: understand the microkernel's lifeblood — IPC. Follow a single
> `open("/a.txt")` through the complete three-party collaboration between
> the client, the kernel, and the server. This is the key to understanding
> every upper-level facility of EwokOS (the filesystem, the window system,
> the network).
> This chapter is code set #7 on the hands-on main line: the essence of a
> microkernel operating system.
> Terminology (IPC/RPC/proto_t/ipc_task_t/shm/DMA…) is in the
> [glossary](99-glossary.md).

## 9.1 Why a Microkernel Must Solve Communication

Ch. 01 said it: the filesystem, the drivers, and the window system are all
independent user-mode processes. They must call each other, for example:

- `shell` must call `vfsd` to read files;
- `vfsd` must call `sdfsd` to read the SD card;
- a window program must call `xserverd` to draw.

### What Are the Ways to Implement IPC?

Processes are isolated into different memory worlds by page tables
(Ch. 05). To exchange data, the classic routes are:

| Method | Principle | Characteristics |
|---|---|---|
| Shared memory | Both sides' page tables map the same physical pages; read/write directly | Fastest (zero-copy), but you must solve synchronization yourself |
| Pipes / message queues | The kernel maintains a buffer; one side writes, the other reads | Simple, but "when to come pick up the goods" needs extra notification |
| Signals | Short one-shot notifications | Can only carry tens of bytes of "event", not data |
| Message passing (RPC, Remote Procedure Call) | One side sends a request and blocks; the other handles it and replies | Naturally synchronous, naturally carries service semantics |

Note one fact: **none of these methods can bypass the kernel** — only the
kernel can see both processes' page tables at once, and only the kernel can
wake/block processes. So "inter-process communication" is essentially
**passing things by the kernel's hand**; the only differences are what is
passed and how.

EwokOS's choice: **message passing + RPC with synchronous semantics**. It
does not additionally provide ports, pipes, or message queues — the kernel
itself **is** the message broker.

```
┌─────────────────────┐                        ┌──────────────────────────────┐
│ client process       │                        │ server process                │
│                     │  SYS_IPC_CALL          │                              │
│ ipc_call(pid, cmd,  │ ─────────────────────► │ the ipc entry function       │
│        in, out)     │   kernel allocates an  │   ipc_get_arg(uid)           │
│                     │   ipc task             │   ... handle cmd ...         │
│ blocked, waiting    │                        │   ipc_set_return(uid, out)   │
│                     │ ◄───────────────────── │   ipc_end()                  │
│ ipc_get_return()    │  kernel wakes client   │                              │
└─────────────────────┘                        └──────────────────────────────┘
```

One call = the client blocks + the server executes + the client is woken.
Simple, predictable, used like a local function call.

## 9.2 The Server Side: How to Become a Service

Any process becomes a server by calling the `ipc_serv_run()` main loop
([ipc_serv.c](../../system/basic/libc/libewoksys/ewoksys/src/ipc_serv.c)).
The framework is roughly:

```c
// Pseudocode: a service process's main loop
void ipc_serv_run(ipc_serv_t* serv, void(*handler)(ipc_serv_t*, int32_t uid)) {
    while(1) {
        uid = ipc_serv_wait();       // sleep until a request arrives
        handler(serv, uid);          // handle the request
        ipc_end(uid);                // tell the kernel: this request is done
    }
}
```

**How does a request "arrive"?** Recall Ch. 07's context hijacking: after
the client issues the call, the kernel marks the server process as pending;
the next time the server is scheduled, it starts executing directly from
the registered **ipc entry function**, with the argument (the task number
`uid`) passed in a register. After handling, `ipc_end()` makes the kernel
restore its original context and resume the main loop — it cannot even
perceive that a switch happened.

### Single-Task Mode vs. Multi-Task Mode

- **Single-task mode (default)**: the whole process handles only one
  request at a time; while it's busy, other clients' requests queue up (the
  kernel puts the callers to sleep with `IPC_ERROR_RETRY` semantics).
  Simple, but a slow request blocks everyone;
- **Multi-task mode (`IPC_MULTI_TASK`)**: the kernel maintains a **worker
  thread pool** for the service, and multiple requests are dispatched to
  different threads for parallel handling. High-concurrency services like
  the filesystem and the window server use this.

## 9.3 The Client Side: How to Call a Service

Clients use the library function `ipc_call`
([ipc.c](../../system/basic/libc/libewoksys/ewoksys/src/ipc.c)):

```c
proto_t in, out;
proto_init(&in, buf_in, sizeof(buf_in));
proto_init(&out, buf_out, sizeof(buf_out));

proto_add_int(&in, arg1);                     // pack the arguments
int res = ipc_call(server_pid, cmd_no, &in, &out);  // ★ synchronously wait for the result
if(res == 0) {
    int value = proto_read_int(&out);   // unpack the result
}
```

Data is packed with `proto_t`
([proto.c](../../system/basic/libc/libewoksys/ewoksys/src/proto.c)): a
length-prefixed byte packet supporting integers, strings, and binary blobs.
Why not pass structs directly? Because the two processes' memory spaces are
mutually invisible — the data must first be **copied into a kernel staging
area** and then copied into the other side's space. Packing into a byte
stream is the simplest and most reliable.

### The Efficiency of Inter-Process Data Exchange

Be honest about the cost: an ordinary `ipc_call`'s data goes through at
least two copies of **user → kernel → user**, plus two context switches.
For small requests like "open a file", the overhead is negligible; but for
big data like "send an 800×480 bitmap to the display driver", copying is a
disaster.

EwokOS's answer is **tiering**:

| Data size | Means | Number of copies |
|---|---|---|
| tens of bytes of arguments | `proto_t` messages | 2 — no big deal |
| large blocks (images / buffers) | **shared memory** (`SYS_PROC_SHM_*`): both sides' page tables map the same physical pages; the IPC only passes a "handle" | 0 |
| buffers needing direct kernel/device access | contiguous physical memory mapping (DMA scenarios) | 0 |

Remember the microkernel efficiency rule: **control signals travel as
messages; big data travels through shared memory**. Ch. 13's graphics
system (xwin) pushes exactly these two to the extreme — every window's
pixels live in shared memory, and IPC only carries short messages like
"which canvas got dirty".

## 9.4 The Kernel Side: IPC's Real Data Structures

Now dive into the guts of [ipc.c](../../kernel/kernel/src/ipc.c). The
microkernel's essence is not the concept of "messages" but **the state
machine the kernel maintains for every service process**. Understand the
three structs below and you have truly read EwokOS's IPC.

### ① One Request = One Task Slot (ipc_task_t)

Every call a client makes occupies one **task slot** on the server side:

```c
typedef struct {
    uint32_t  uid;          // this call's global number (allocated incrementally by the kernel)
    uint32_t  counter;
    uint32_t  state;        // IPC_BUSY: being handled
    proto_t   arg_ret;      // ★one buffer shared by the arguments and the return value
    int32_t   client_pid;   // who called me
    uint32_t  client_uuid;  // a second check against pid reuse
    uint8_t   client_intr;  // whether the caller is in an interrupt context
    int32_t   call_id;      // the command number (including flags like IPC_LAZY)
    int32_t   handler_pid;  // multi-task mode: which worker thread is serving me
    uint32_t  handler_uuid;
} ipc_task_t;
```

Key point: the `arg_ret` buffer **first holds the arguments the client
sent, and is overwritten by the result after handling**. In other words,
"arguments" and "return value" reuse the same memory, with `state`
distinguishing the phases.

### ② A Service's Ledger (ipc_server_t)

Every process registered as a service has an `ipc_server_t` embedded in
its `space`:

```c
typedef struct {
    int32_t       lock;          // SMP spinlock (for simultaneous multicore calls)
    bool          disabled;      // stop serving (temporarily refuse new requests during upgrade/maintenance)
    bool          multi_task;    // single task? or a thread pool?
    ewokos_addr_t entry;         // ★the address of the ipc entry function (the springboard for context hijacking)
    ipc_task_t    tasks[IPC_CTX_MAX];  // the task-slot array; IPC_CTX_MAX = 8
    uint8_t       task_head, task_tail, task_num;   // ring queue head/tail
    ipc_pool_worker_t* pool;     // multi-task mode's worker thread pool
    bool          do_switch;     // single-task mode: the kernel has hijacked the context, switch pending
    ewokos_addr_t stack;         // the dedicated kernel stack used during hijacking
    saved_state_t saved_state;   // ★the snapshot of the original context before hijacking
    ipc_queue_item_t* wait_head, wait_tail;  // clients queue here when the server is busy
} ipc_server_t;
```

Three things worth remembering:

1. **The task slots are a fixed-length array** (8), not dynamically
   allocated — avoiding `kmalloc` while the service is busy; when full,
   later clients simply queue;
2. **`entry` is the hijacking springboard**: after a client calls, the
   kernel makes the server process start executing from the `entry`
   address, instead of continuing its original `main` loop;
3. **`saved_state` is the restore point**: before hijacking, the server's
   original register scene is saved here, and `ipc_end()` restores it —
   this is the concrete implementation of Ch. 07's "context hijacking".

### ③ How Arguments Cross Processes: the proto_t Byte Packet

Two processes' memory is mutually invisible, so arguments must be **copied
through the kernel**. `proto_t`
([proto.c](../../system/basic/libc/libewoksys/ewoksys/src/proto.c)) is a
"self-describing byte stream":

```c
typedef struct {
    void*    data;        // the data buffer (small data uses the built-in buffer; big data mallocs)
    uint32_t size;        // bytes written so far
    uint32_t total_size;
    uint32_t offset;      // how far we've read
} proto_t;
```

- Writing: `proto_add_int(&p, x)` appends an integer at fixed length to the
  tail of `data`;
- Reading: `proto_read_int(&p)` takes it sequentially from `offset` and
  advances the offset;
- **When crossing processes**, the kernel uses `proto_copy` to copy the
  whole `data` segment into the other side's space.

Why not pass structs directly? Because pointers inside a struct are invalid
in the other side's address space — they must be flattened into a
"pure-data" byte stream. Packing into a byte stream is the simplest, most
reliable, and most portable.

## 9.5 The Kernel Side: a Call's Complete Journey

Now string the data structures into a flow. `SYS_IPC_CALL` enters the
kernel's `proc_ipc_call` ([ipc.c](../../kernel/kernel/src/ipc.c)) and
proceeds in two modes.

### Single-Task Mode (default; most drivers)

```
A: SYS_IPC_CALL(B, cmd, arg)
   ├─ kernel proc_ipc_req: occupy a slot at the tail of B's task-slot ring
   │   queue; proto_copy copies A's arguments into the slot; if full, hang A
   │   on the wait queue
   ├─ kernel proc_ipc_do_task:
   │   ① save B's current context into B->saved_state (the restore point)
   │   ② set B->do_switch = true
   │   ③ proc_switch hands the CPU straight to B
   └─ A blocks, waiting for the reply
B: scheduled → starts executing from entry (the ipc entry), with the task
   number uid in a register
   ├─ handles the request…
   ├─ SYS_IPC_SET_RETURN: writes the result into the slot's arg_ret
   └─ SYS_IPC_END → proc_ipc_end:
       ① restore B's original context from saved_state (B returns to its own main loop)
       ② close the slot, copy the result to A, wake A
       ③ if more requests are queued, continue with the next one directly
A: wakes from the block; ipc_get_return picks up the result
```

**Note the cost of single-task mode**: B's entire process serves only one
request at a time; a slow request blocks everyone behind it. It's just
right for "in and out fast" services like drivers.

### Multi-Task Mode (`IPC_MULTI_TASK`; xserverd / the filesystem)

High-concurrency services cannot wait serially. The kernel maintains a
**worker thread pool** for them:

```
A: SYS_IPC_CALL(B, cmd, arg)
   ├─ proc_ipc_req: grab any free slot among B's task slots (order not required)
   ├─ ipc_pool_assign: find an idle worker thread bound to A's core;
   │   if none, hatch a new thread on demand (capped at the process's thread limit)
   └─ proc_switch hands the CPU straight to that worker thread → the request starts running
B's worker thread: executes entry → handles → ipc_end → the thread "parks" for reuse
   └─ if still idle after 3 seconds, the thread exits automatically and returns its slot
A: woken up and picks up the result
```

Two variants to tell apart:

- **`IPC_MULTI_TASK`**: may hatch multiple worker threads to handle
  requests in parallel; clients queue for threads;
- **`IPC_MULTI_CORE`**: **at most one** worker thread per CPU core; if the
  core is busy, it immediately returns `IPC_ERROR_RETRY` for the caller to
  retry — thriftier with threads, better for "per-core independent"
  scenarios.

The thread pool's design philosophy (stated clearly in the source
comments): **the pool starts empty, hatches on demand, parks and reuses
after use, and shrinks back to empty when idle**. It neither wastes memory
nor preheats unneeded threads.

### Error Codes

[syscalls.h](../../kernel/kernel/include/syscalls.h) defines three
sentinel values, all returned to userland via `ctx->gpr[0]`:

```c
#define IPC_ERROR_RETRY      -1   // server busy/disabled; the kernel blocks the caller for a later retry
#define IPC_ERROR_SELF       -2   // cannot call yourself (would deadlock)
#define IPC_ERROR_NO_READY   -3   // the peer hasn't registered an ipc service yet
```

`IPC_ERROR_RETRY` is not a "failure" but a "come back in a moment" — libc
automatically re-issues the call after being woken. This is the most common
flow-control protocol in a microkernel.

## 9.6 How to Find a Service: the Name Service (core)

Calling requires the peer process's pid, but "what is the filesystem
service's pid?" may differ on every boot. EwokOS uses the `core` process as
a **name service**:

```
when a service starts:  ipc_serv_reg("vfs", service_id)  → core records "vfs → pid 3"
when a client calls:    get_serv_pid("vfs")              → asks core → gets pid 3
```

`core` is also responsible for system-level coordination (like the current
directory and the hostname), so it must be **the second process to
start** — exactly why `init` in Ch. 11 starts `/sbin/core` first.

## 9.7 Re-reading "Reading a File" Through IPC Eyes

Now you can fully explain the diagram from Ch. 01:

```
app: open("/etc/passwd")
  → libc: ipc_call(vfsd_pid, VFS_CMD_OPEN, ...)      【IPC hop 1】
  → vfsd: consults the mount table, finds "/" is served by sdfsd
  → vfsd: ipc_call(sdfsd_pid, FS_CMD_OPEN, ...)      【IPC hop 2】
  → sdfsd: accesses the SD card controller via SYS_MEM_MAP to read sectors,
           parses the ext2 directories, finds the inode
  → the result returns along the same path; the app gets a file handle
```

**The kernel merely ferries messages the whole time.** Even if the
filesystem code were all bugs, the only thing to crash would be the single
`sdfsd` process.

## 9.8 The Microkernel: Communication Is Service

At this point we can distill the microkernel's worldview:

> **In a microkernel, a "service" is not a piece of code inside the kernel
> but "a process that can be called". To use any capability, send that
> process a message. Communication is service.**

Corollary one: **every upper-level facility is the same pattern**. The
filesystem is the process `vfsd`, the window system is the process
`xserverd`, the network is the process `netd` — their "APIs" are uniformly
one `ipc_call`. Learning Ch. 09 means having learned the skeleton of Ch. 10,
Ch. 13, and the network chapter (in the next volume) at the same time.

Corollary two: **services can crash, can restart, and can even be
upgraded**. If the server dies while a client is calling, the kernel only
returns an error code, and the client decides whether to retry or give up;
the system doesn't die with it. This is resilience a monolithic kernel
cannot offer.

Corollary three: **services can also be replaced**. As long as the protocol
(command numbers and argument formats) stays the same, `vfsd`'s
implementation can be swapped wholesale and no other process will notice.
This is the decoupling brought by a "message-oriented architecture".

Looking back at the whole tutorial: the processes, privilege levels, system
calls, and scheduling built in chapters 03~08 all exist to support this
chapter's one sentence — **let mutually isolated processes safely serve
each other**. That is the essence of a microkernel operating system.

## 9.9 Exercises

1. After booting, run `svcinfo` (or cross-check `ps` with the boot log) and
   find all the service processes in the system (`uartd`, `vfsd`,
   `timerd`…);
2. Find the service registry in
   [core.c](../../system/basic/sys/core/core.c) and see how names like
   `vfs` and `sd` map to pids;
3. Food for thought: if the server process crashes while a client is
   calling it, what happens? (Hint: the kernel wakes the waiting clients
   and returns an error — find this logic in `ipc.c`.)

## 9.10 Summary

- The ways to implement IPC: shared memory / pipes / signals / message
  passing — all completed by the kernel's hand;
- EwokOS IPC is synchronous RPC: the client blocks, the kernel relays, the
  server executes, the result wakes;
- The kernel maintains an `ipc_server_t` per service: a fixed array of 8
  task slots, the hijacking springboard `entry`, the restore point
  `saved_state`, and a wait queue for busy times; each request is one
  `ipc_task_t` slot;
- Single-task mode serializes via "context hijacking";
  `IPC_MULTI_TASK`/`IPC_MULTI_CORE` parallelize with a worker thread pool
  that hatches on demand and parks for reuse;
- Arguments transit as `proto_t` self-describing byte streams; the
  efficiency rule: small data as messages, big data as shared memory;
- The `core` process provides the name service, decoupling service
  discovery from pids;
- The microkernel worldview: communication is service — the filesystem,
  windows, and networking are all built on top of it.

Next chapter: follow `sdfsd`'s path downward — how the SD card and the
filesystem are implemented.
