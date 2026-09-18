#include <ewoksys/cap.h>
#include <ewoksys/proc.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Capability syscall smoke test. Run as root (uid 0, holds CAP_ROOT).
 * Prints one line per check and a final verdict; exits 0 on success.
 */
static int _fail = 0;

static void check(int ok, const char* name) {
    printf("  [%s] %s\n", ok ? " ok " : "FAIL", name);
    if(!ok)
        _fail = 1;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    printf("captest: start\n");

    /* own sid is non-zero and stable across queries */
    cap_info_t info;
    check(cap_get(-1, &info) == 0, "cap_get(sid)");
    check(info.sid != 0, "sid non-zero");
    uint64_t sid = info.sid;
    check(cap_get(-1, &info) == 0 && info.sid == sid, "sid stable");

    /* a root shell holds a full-rights CAP_ROOT cap somewhere in its cnode */
    int root_slot = -1;
    for(int i = 0; i < CNODE_SLOTS; i++) {
        if(cap_get(i, &info) == 0 && info.cap.type == CAP_ROOT) {
            root_slot = i;
            break;
        }
    }
    check(root_slot >= 0, "CAP_ROOT present");
    check(root_slot >= 0 &&
            (info.cap.rights & (CAP_R|CAP_W|CAP_X|CAP_GRANT)) ==
                    (CAP_R|CAP_W|CAP_X|CAP_GRANT),
            "CAP_ROOT full rights");

    /* mint a frame cap for the first 4K of physical RAM */
    int frame_slot = cap_mint(CAP_FRAME, CAP_R | CAP_W, 0x40000000ULL, 0x1000ULL);
    check(frame_slot >= 0, "mint CAP_FRAME");
    check(frame_slot >= 0 && cap_get(frame_slot, &info) == 0 &&
            info.cap.type == CAP_FRAME &&
            info.cap.obj.frame.paddr == 0x40000000ULL &&
            info.cap.obj.frame.size == 0x1000ULL,
            "frame cap object");

    /* CAP_ROOT itself is kernel-issued only */
    check(cap_mint(CAP_ROOT, CAP_R, 0, 0) < 0, "CAP_ROOT not mintable");

    /* mint a process cap on ourselves, then forward it to ourselves */
    int proc_slot = cap_mint(CAP_PROCESS, CAP_W | CAP_GRANT, getpid(), 0);
    check(proc_slot >= 0, "mint CAP_PROCESS(self)");
    int granted = cap_grant(getpid(), proc_slot, CAP_W); /* attenuate: drop GRANT */
    check(granted >= 0, "grant to self");
    check(granted >= 0 && cap_get(granted, &info) == 0 &&
            info.cap.type == CAP_PROCESS &&
            info.cap.obj.proc.pid == getpid() &&
            (info.cap.rights & CAP_W) != 0 &&
            (info.cap.rights & CAP_GRANT) == 0,
            "granted cap attenuated");

    /*
     * Re-granting a cap that lacks CAP_GRANT: a CAP_ROOT holder bypasses
     * the gate by design (only non-root callers are denied).
     */
    check(cap_grant(getpid(), granted, 0) >= 0, "root bypasses CAP_GRANT gate");

    /* revoke works and the slot goes back to INVALID */
    check(cap_revoke(proc_slot) == 0, "revoke");
    check(cap_get(proc_slot, &info) == 0 && info.cap.type == CAP_INVALID,
            "slot cleared");

    /* fork inherits the parent's cnode */
    int32_t pid = fork();
    if(pid == 0) {
        cap_info_t cinfo;
        int ok = (cap_get(granted, &cinfo) == 0 &&
                cinfo.cap.type == CAP_PROCESS &&
                cinfo.cap.obj.proc.pid == getppid());
        printf("  [%s] fork inherits cnode\n", ok ? " ok " : "FAIL");
        _exit(ok ? 0 : 1);
    }
    int status = 0;
    wait(&status);
    check(status == 0, "child check passed");

    printf("captest: %s\n", _fail ? "FAILED" : "ALL PASSED");
    return _fail;
}
