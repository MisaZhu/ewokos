#ifndef SUDOD_H
#define SUDOD_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * sudo / sudod client-server protocol.
 *
 * sudod is the privileged daemon (holds CAP_ROOT via /etc/cap.json) that
 * actually authorizes the caller against /etc/sudoers, authenticates the
 * caller's password through sessiond, and spawns the requested command as the
 * target user. The unprivileged /bin/sudo client only resolves the command,
 * collects credentials, and relays the terminal I/O over shm pipes.
 *
 * Request (SUDOD_EXEC), in proto order:
 *   adds  target_user   - user to run as ("root" by default)
 *   adds  prog          - resolved absolute command path ("" => target's shell)
 *   adds  args          - space-joined arguments after the command
 *   adds  password      - caller's password ("" on the first attempt)
 *   addi  shm_out       - shm ring id of the stdout pipe (client is reader)
 *   addi  shm_err       - shm ring id of the stderr pipe (client is reader)
 *   addi  shm_ctl       - shm ring id of the control pipe (client is reader)
 *
 * Reply:
 *   addi  status        - one of SUDOD_* below
 *   addi  shm_in        - shm ring id of the stdin pipe (on SUDOD_OK; the
 *                         client attaches the write end with vfs_pipe_attach)
 */

enum {
	SUDOD_EXEC = 0,
};

#define SUDOD_OK             0
#define SUDOD_ERR_DENY      (-1)   /* sudoers does not allow this request */
#define SUDOD_NEED_PASSWORD (-2)   /* password required: retry with it */
#define SUDOD_ERR_PASSWORD  (-3)   /* authentication failed */
#define SUDOD_ERR_USER      (-4)   /* unknown target user */
#define SUDOD_ERR_INTERNAL  (-5)   /* spawn/pipe failure */

#define IPC_SERV_SUDOD "ipc_serv.sudod"

#ifdef __cplusplus
}
#endif

#endif
