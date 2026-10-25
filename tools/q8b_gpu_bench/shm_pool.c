/* Exhaust available 8MB chunks of the real contiguous pool, including
 * reuse after exhaustion. Run separately from input latency measurements. */
#include <sys/shm.h>
#include <ewoksys/shm.h>
#include <ewoksys/sys.h>
#include <ewoksys/klog.h>
#include <stdint.h>
#include <unistd.h>

#define CHUNK (8u * 1024u * 1024u)
#define MAX_CHUNKS 64

int main(void) {
    sys_info_t info;
    if (sys_get_sys_info(&info) != 0 || !info.shm_contig.size)
        return 1;
    klog("shm-pool base=%llx size=%u page=%u\n",
            (unsigned long long)info.shm_contig.phy_base,
            info.shm_contig.size, info.page_size);
    for (unsigned round = 0; round < 3; round++) {
        int ids[MAX_CHUNKS];
        volatile uint32_t* ptr[MAX_CHUNKS];
        unsigned count = 0, errors = 0;
        unsigned zero_errors = 0, phys_errors = 0, data_errors = 0;
        while (count < MAX_CHUNKS) {
            /* EwokOS IPC_PRIVATE resets flags to 0666, so use exclusive
             * per-process keys to preserve IPC_CONTIG. */
            key_t key = 0x60000000u + (unsigned)getpid()*256u + round*64u + count;
            int id = shmget(key, CHUNK, IPC_CREAT | IPC_EXCL | IPC_CONTIG | 0600);
            if (id < 0) break;
            ids[count] = id;
            if (shmctl(id, IPC_SHM_IS_CONTIG, 0) != 1) {
                shmctl(id, IPC_RMID, 0);
                errors++;
                break;
            }
            ptr[count] = shmat(id, 0, 0);
            if (!ptr[count] || ptr[count] == (void*)-1) {
                shmctl(id, IPC_RMID, 0);
                errors++;
                break;
            }
            uint64_t phys = shm_contig_phy_addr(id, (ewokos_addr_t)ptr[count]);
            if (phys < info.shm_contig.phy_base ||
                    phys + CHUNK > info.shm_contig.phy_base + info.shm_contig.size)
                phys_errors++;
            for (unsigned offset = 0; offset < CHUNK / 4; offset += info.page_size / 4) {
                if (ptr[count][offset] != 0) {
                    if (zero_errors < 3) klog("shm-pool nonzero chunk=%u offset=%u value=%x phys=%llx\n",
                            count, offset*4, ptr[count][offset], (unsigned long long)phys);
                    zero_errors++;
                }
                ptr[count][offset] = 0x12340000u ^ (round << 12) ^ count ^ offset;
            }
            count++;
        }
        for (unsigned i = 0; i < count; i++) {
            for (unsigned offset = 0; offset < CHUNK / 4; offset += info.page_size / 4)
                if (ptr[i][offset] != (0x12340000u ^ (round << 12) ^ i ^ offset))
                    data_errors++;
            shmdt((void*)ptr[i]);
            shmctl(ids[i], IPC_RMID, 0);
        }
        errors += zero_errors + phys_errors + data_errors;
        klog("shm-pool round=%u chunks=%u MB=%u errors=%u zero=%u phys=%u data=%u\n",
                round, count, count * 8u, errors, zero_errors, phys_errors, data_errors);
        if (errors || !count) return 1;
    }
    klog("shm-pool DONE rounds=3\n");
    return 0;
}
