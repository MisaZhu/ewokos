/*
 * mount.c - mount table and mount/umount operations.
 */
#include "vfsd.h"

mount_t _vfs_mounts[FS_MOUNT_MAX];

/*
 * vfsd-internal reverse map: mounted fs-root node id per mount slot.
 * mount_t only records org_node (the covered mount-point id), so exit-time
 * teardown needs this to find the subtree root that carries mount_id.
 * 0 means the slot is free. Guarded by _vfs_lock exactly like _vfs_mounts.
 */
static uint32_t _vfs_mount_node[FS_MOUNT_MAX];

/* caller must hold _vfs_lock (read or write) */
static int32_t vfs_get_mount_id(vfs_node_t* node) {
    while(node != NULL) {
        if(node->mount_id >= 0)
            return node->mount_id;
        node = node->father;
    }
    return -1;
}

/* caller must hold _vfs_lock (read or write) */
static int32_t vfs_get_mount(vfs_node_t* node, mount_t* mount) {
    if(node == NULL || mount == NULL)
        return -1;

    int32_t mount_id = vfs_get_mount_id(node);
    if(mount_id < 0)
        return -1;
    memcpy(mount, &_vfs_mounts[mount_id], sizeof(mount_t));
    return 0;
}

/* caller must hold _vfs_lock (read or write) */
int32_t get_mount_pid(vfs_node_t* node) {
    mount_t mount;
    int32_t res = vfs_get_mount(node, &mount);
    if(res == 0)
        return mount.pid;
    return -1;
}

/*
 * Who may detach a mount: the driver that created it, or root. Drivers run as
 * the 'sys' user and unmount their own node on teardown (device_run ->
 * vfs_umount); root is additionally allowed so a privileged umount tool can
 * detach a mount whose owning daemon is gone or does not cooperate. The 'sys'
 * driver user alone is NOT enough here - that would let any driver tear down
 * another driver's mount.
 * caller must hold _vfs_lock (read or write)
 */
static inline int32_t check_mount(int32_t pid, vfs_node_t* node) {
    int32_t mnt_pid = get_mount_pid(node);
    if(mnt_pid == pid) //the mounting proc itself.
        return 0;

    procinfo_t procinfo;
    if(proc_info(pid, &procinfo) == 0 && procinfo.uid <= 0) //root.
        return 0;
    return -1;
}

/* caller must hold _vfs_lock (read or write) */
static int32_t vfs_get_free_mount_id(void) {
    int32_t i;
    for(i = 0; i<FS_MOUNT_MAX; i++) {
        if(_vfs_mounts[i].org_node == 0)
            return i;
    }
    return -1;
}

/* caller must hold _vfs_lock (read or write) */
int32_t vfsd_get_mount_by_id(int32_t id, mount_t* mount) {
    if(mount == NULL)
        return -1;

    if(id < 0 || id >= FS_MOUNT_MAX ||
            _vfs_mounts[id].org_node == 0)
        return -1;
    memcpy(mount, &_vfs_mounts[id], sizeof(mount_t));
    return 0;
}

/*
 * Who may mount over 'org': root, or the owner of the covered node. Every
 * mount point in the shipped configs (/dev/..., /tmp) is created by the very
 * driver that then mounts onto it (vfs_create(..., node_only) records
 * stat.uid = getuid()), and sdfsd covers the initial "/" as root, so this is
 * compatible with drivers running as the 'sys' user. W_OK on the covered
 * node is accepted too (writable directory). Without any check ANY process
 * could mount its own node over "/" or "/dev/tty0" and hijack the whole
 * namespace for everyone.
 * caller must hold _vfs_lock (read or write)
 */
int32_t vfsd_check_mount_access(int32_t pid, vfs_node_t* org) {
    if(org == NULL)
        return -1;
    if(vfsd_check_owner(pid, &org->fsinfo) == 0)
        return 0;
    if(vfsd_check_access(pid, &org->fsinfo, W_OK) == 0)
        return 0;
    return -1;
}

/* caller must hold _vfs_lock (write) */
int32_t vfsd_mount(int32_t pid, vfs_node_t* org, vfs_node_t* node, const char* desc) {
    if(org == NULL || node == NULL || org == node)
        return -1;

    if(node->mount_id >= 0) //already been mounted
        return -1;

    /*
     * Structural guards - node/org ids come straight from the client:
     *  - 'node' must be detached (fresh vfs_new_node with father 0). Linking
     *    an already-attached node into a second kid list without removing it
     *    from the first corrupts both lists (a later walk/free crashes).
     *  - 'node' must not be the live root: grafting the root under one of its
     *    own descendants makes a cycle and every father-walk loops forever.
     *  - a detached 'org' is only acceptable when it IS the root; otherwise
     *    the father==NULL branch below would let any peer replace _vfs_root
     *    with an arbitrary node.
     */
    if(node->father != NULL || node == _vfs_root)
        return -1;
    if(org->father == NULL && org != _vfs_root)
        return -1;

    int32_t id = vfs_get_free_mount_id();
    if(id < 0)
        return -1;

    char org_name[FS_FULL_NAME_MAX];
    vfsd_fullname(org, org_name, sizeof(org_name));

    _vfs_mounts[id].pid = pid;
    _vfs_mounts[id].org_node = vfs_get_node_id(org);
    _vfs_mount_node[id] = vfs_get_node_id(node);
    strcpy(_vfs_mounts[id].org_name, org_name);
    /* desc comes from proto_read_str, which yields NULL for an absent OR
     * empty string; strncpy(dst, NULL) would crash vfsd. Guard it and always
     * NUL-terminate the fixed-width field. */
    if(desc != NULL)
        strncpy(_vfs_mounts[id].desc, desc, DESC_MAX-1);
    else
        _vfs_mounts[id].desc[0] = 0;
    _vfs_mounts[id].desc[DESC_MAX-1] = 0;
    strcpy(node->fsinfo.name, org->fsinfo.name);
    node->mount_id = id;

    vfs_node_t* father = org->father;
    if(father == NULL) {
        _vfs_root = node;
    }
    else {
        vfs_remove(pid, org);
        vfs_add_node(pid, father, node);
    }
    return 0;
}

/* caller must hold _vfs_lock (write) */
static void vfs_umount_now(vfs_node_t* node) {
    if(node == NULL || node->mount_id < 0 || node->mount_id >= FS_MOUNT_MAX)
        return;

    int32_t id = node->mount_id;
    vfs_node_t* org = vfs_get_node_by_id(_vfs_mounts[id].org_node);
    if(org == NULL) {
        return;
    }

    vfs_node_t* father = node->father;
    if(father == NULL) {
        /* only the live root may be swapped back; a detached mounted root
         * (already removed from its father) has nowhere to relink 'org' */
        if(node == _vfs_root)
            _vfs_root = org;
    }
    else {
        vfs_remove(0, node);
        if(org->mount_id < 0) {
            /*
             * The covered placeholder is dropped. It must go through
             * vfsd_del_node(): a bare free() left it in _nodes_hash and in
             * clients' hands (any peer that resolved the mount point before
             * the mount still holds its id), so the next VFS_GET_BY_NODE on it
             * was a use-after-free inside vfsd. vfsd_del_node also wakes its
             * waiters and refuses (keeps it allocated) while it is still open.
             * Clear the stale father first: vfsd_mount() unlinked org from
             * that kid list long ago, so letting the delete "unlink" it again
             * would corrupt the father's kids_num.
             */
            org->father = NULL;
            vfsd_del_node(org);
        }
        else
            vfs_add_node(0, father, org);
    }
    _vfs_mount_node[id] = 0;
    memset(&_vfs_mounts[id], 0, sizeof(mount_t));
    /*
     * The unmounted subtree no longer belongs to any slot. Leaving mount_id
     * pointing at the freed slot made the detached tree inherit whatever
     * driver mounts into that slot next (get_mount_pid routed to it, and that
     * driver's umount could then re-run this teardown on our node).
     */
    node->mount_id = -1;
    node->pending_umount = 0;
}

/* caller must hold _vfs_lock (write) */
void vfs_try_finish_umount(vfs_node_t* node) {
    if(node == NULL || node->mount_id < 0 || !node->pending_umount)
        return;
    if(node->refs > 0 || node->refs_w > 0)
        return;
    vfs_umount_now(node);
}

/* caller must hold _vfs_lock (write) */
void vfsd_umount(int32_t pid, vfs_node_t* node) {
    if(node == NULL || node->mount_id < 0 || check_mount(pid, node) != 0)
        return;

    if(node->refs > 0 || node->refs_w > 0) {
        node->pending_umount = 1;
        return;
    }

    vfs_umount_now(node);
}

/*
 * Force-tear down every mount owned by a process that is going away.
 *
 * A filesystem driver that dies without calling vfs_umount() would otherwise
 * leave its mount-table slot owned by a dead pid and keep the mounted subtree
 * wired into the namespace: every later path lookup under the mount point
 * routes IPC to the dead mount_pid and stalls/fails, and the slot is never
 * reclaimed (FS_MOUNT_MAX is only 32). Called from clear_zombie() under the
 * write lock, this mirrors vfsd_umount()'s per-mount semantics: detach now
 * when the mounted root is idle, otherwise mark pending_umount so teardown
 * completes once the last open descriptor drops its ref. Either way the
 * subtree nodes stay allocated and valid for those surviving descriptors.
 *
 * caller must hold _vfs_lock (write)
 */
void vfs_umount_by_pid(int32_t pid) {
    if(pid <= 0)
        return;

    int32_t i;
    for(i=0; i<FS_MOUNT_MAX; i++) {
        if(_vfs_mounts[i].org_node == 0 || _vfs_mounts[i].pid != pid)
            continue;

        vfs_node_t* node = vfs_get_node_by_id(_vfs_mount_node[i]);
        if(node == NULL || node->mount_id != i) {
            /* mounted root already gone: just reclaim the slot */
            _vfs_mount_node[i] = 0;
            memset(&_vfs_mounts[i], 0, sizeof(mount_t));
            continue;
        }

        if(node->refs > 0 || node->refs_w > 0) {
            node->pending_umount = 1;
            continue;
        }
        vfs_umount_now(node);
    }
}
