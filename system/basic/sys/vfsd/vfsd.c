/*
 * vfsd.c - virtual file system server: global state init and main loop.
 * See vfsd.h for the module layout and the concurrency model.
 */
#include "vfsd.h"

pthread_rwlock_t _vfs_lock;

static void vfsd_init(void) {
    uint32_t i;
    for(i = 0; i<FS_MOUNT_MAX; i++) {
        memset(&_vfs_mounts[i], 0, sizeof(mount_t));
    }

    sys_info_t sysinfo;
    sys_get_sys_info(&sysinfo);
    _max_proc_table_num = sysinfo.max_task_num;
    _proc_fds_table = (proc_fds_t*)malloc(_max_proc_table_num*sizeof(proc_fds_t));

    for(i = 0; i<_max_proc_table_num; i++) {
        memset(&_proc_fds_table[i], 0, sizeof(proc_fds_t));
    }

    queue_init(&_zombie_tasks);
    pthread_mutex_init(&_driver_async_worker.lock, NULL);
    queue_init(&_driver_async_worker.jobs);
    pthread_mutex_init(&_driver_kids_results_lock, NULL);
    queue_init(&_driver_kids_results);
    pthread_rwlock_init(&_vfs_lock, NULL);
    _nodes_hash = hashmap_new(0);
    _vfs_root = vfsd_new_node();
    strcpy(_vfs_root->fsinfo.name, "/");
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if(ipc_serv_reg(IPC_SERV_VFS) != 0) {
        klog("reg vfs ipc_serv error!\n");
        return -1;
    }

    vfsd_init();
    start_driver_async_worker();
    /*
     * IPC_MULTI_CORE: every request is served by its own kernel-spawned
     * worker thread inside this proc; all shared state is guarded by
     * _vfs_lock (see the lock rules in vfsd.h).
     */
    ipc_serv_run(handle, clear_pending_zombies, NULL, IPC_MULTI_CORE);

    while(true) {
        /*
         * ipc service main processes do not truly sleep in SYS_USLEEP;
         * the kernel only schedules away once and may run them again
         * immediately. Keep vfsd parked with a real block instead of a
         * yield loop so it cannot burn CPU while its service threads do
         * the actual work.
         */
        proc_block();
    }

    free(_proc_fds_table);
    return 0;
}
