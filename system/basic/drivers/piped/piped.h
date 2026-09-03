#ifndef PIPED_H
#define PIPED_H

/*
 * Wire protocol of the standalone pipe driver (piped).
 *
 * piped mounts PIPE_DEV_PATH and owns every anonymous pipe created by
 * pipe()/pipe2(): lifecycle refcounting (open/attach/dup/close) and wakeup
 * delivery (poll waiters, close edges). Data transfer never touches piped —
 * it runs on the shared-memory ring described in ewoksys/shm_pipe.h.
 *
 * pipe() protocol:
 *   fd[0] = open(PIPE_DEV_PATH, O_RDONLY)   -> piped creates pipe + shm ring
 *   fd[1] = open(PIPE_DEV_PATH, O_WRONLY)   -> unattached write end
 *   fcntl(fd[1], PIPE_ATTACH, shm_id)       -> bind write end to the pipe
 *
 * The commands below travel as FS_CMD_CNTL (per-fd fcntl) payloads; the
 * fcntl cmd namespace is per-driver, values only need to avoid the generic
 * F_* range.
 *
 * libc (libewoksys/src/vfs.c) carries a minimal inline copy of these
 * constants — keep both sides in sync.
 */

#define PIPE_DEV_PATH "/dev/pipe0"

/* Bind a freshly opened (O_WRONLY) pipe end to an existing pipe.
 * in: [int32 shm_id] — the pipe id returned in fsinfo.data of the read end. */
#define PIPE_ATTACH      0x100

/* Register the caller as a poll waiter on this pipe end's pipe.
 * in: [uint32 events] — VFS_EVT_RD/WR interests. piped wakes registered
 * waiters on data edges (PIPE_EDGE) and on close (VFS_EVT_CLOSE). */
#define PIPE_POLL_WAIT   0x101

/* Drop the caller's poll waiter registration. */
#define PIPE_POLL_UNWAIT 0x102

/* Data-edge notification, sent fire-and-forget by the peer that just moved
 * the ring across the empty/full boundary while poll waiters exist.
 * in: [uint32 events] */
#define PIPE_EDGE        0x103

#endif
