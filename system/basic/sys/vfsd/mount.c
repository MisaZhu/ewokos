/*
 * mount.c - mount table and mount/umount operations.
 */
#include "vfsd.h"

mount_t _vfs_mounts[FS_MOUNT_MAX];

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

static inline int32_t check_mount(int32_t pid, vfs_node_t* node) {
    int32_t mnt_pid = get_mount_pid(node);
    if(mnt_pid != pid) //current proc not the mounting one.
        return -1;
    return 0;
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

/* caller must hold _vfs_lock (write) */
int32_t vfsd_mount(int32_t pid, vfs_node_t* org, vfs_node_t* node, const char* desc) {
    if(org == NULL || node == NULL)
        return -1;

    if(node->mount_id >= 0) //already been mounted
        return -1;

    int32_t id = vfs_get_free_mount_id();
    if(id < 0)
        return -1;

    char org_name[FS_FULL_NAME_MAX];
    vfsd_fullname(org, org_name, sizeof(org_name));

    _vfs_mounts[id].pid = pid;
    _vfs_mounts[id].org_node = vfs_get_node_id(org);
    strcpy(_vfs_mounts[id].org_name, org_name);
    strncpy(_vfs_mounts[id].desc, desc, DESC_MAX-1);
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
    if(node == NULL || node->mount_id < 0)
        return;

    vfs_node_t* org = vfs_get_node_by_id(_vfs_mounts[node->mount_id].org_node);
    if(org == NULL) {
        return;
    }

    vfs_node_t* father = node->father;
    if(father == NULL) {
        _vfs_root = org;
    }
    else {
        vfs_remove(0, node);
        if(org->mount_id < 0)
            free(org);
        else
            vfs_add_node(0, father, org);
    }
    memset(&_vfs_mounts[node->mount_id], 0, sizeof(mount_t));
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
