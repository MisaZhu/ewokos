/* Real shmat/shmdt costs, excluding allocation/zeroing. The parent keeps
 * each segment alive while independent address spaces repeatedly map it.
 * Runs with a desktop: does not open or initialize the GPU. */
#include <sys/shm.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/sys.h>
#include <ewoksys/wait.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define ROUNDS 40u
#define WORKERS 4u

static uint64_t now_ns(void) {
    uint64_t ns = 0;
    if (kernel_tic_nsec(&ns) != 0) exit(2);
    return ns;
}

static uint32_t pattern(unsigned worker, unsigned generation, unsigned offset) {
    return 0x91370000u ^ (worker << 24) ^ (generation << 12) ^ offset;
}

static void report(const char *op, unsigned size, unsigned contig,
        unsigned workers, unsigned worker, uint64_t *samples) {
    uint64_t total = 0;
    for (unsigned i = 0; i < ROUNDS; i++) {
        total += samples[i];
        for (unsigned j = i; j && samples[j] < samples[j-1]; j--) {
            uint64_t tmp = samples[j];
            samples[j] = samples[j-1];
            samples[j-1] = tmp;
        }
    }
    klog("shm-map op=%s bytes=%u contig=%u workers=%u worker=%u n=%u mean_ns=%llu p50_ns=%llu p95_ns=%llu max_ns=%llu\n",
         op, size, contig, workers, worker, ROUNDS,
         (unsigned long long)(total / ROUNDS),
         (unsigned long long)samples[ROUNDS/2],
         (unsigned long long)samples[ROUNDS*95/100],
         (unsigned long long)samples[ROUNDS-1]);
}

static int run(unsigned size, unsigned page, unsigned contig, unsigned workers) {
    int ids[WORKERS], children[WORKERS];
    volatile uint32_t *keepers[WORKERS];
    unsigned created = 0, started = 0, errors = 0;
    for (unsigned w = 0; w < workers; w++) {
        key_t key = 0x61000000u + (unsigned)getpid()*16u + w;
        int id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0600 |
                (contig ? IPC_CONTIG : 0));
        if (id < 0) { errors++; break; }
        volatile uint32_t *p = shmat(id, 0, 0);
        if (!p || p == (void*)-1) {
            shmctl(id, IPC_RMID, 0);
            errors++; break;
        }
        ids[w] = id;
        keepers[w] = p;
        created++;
        for (unsigned off = 0; off < size/4; off += page/4)
            p[off] = pattern(w, 0, off);
        p[size/4-1] = pattern(w, 0, size/4-1);
        p[1] = 0; /* explicit completion/error result, independent of wait ABI */
    }
    if (created == workers) for (unsigned w = 0; w < workers; w++) {
        int pid = fork();
        if (pid < 0) { errors++; break; }
        if (pid == 0) {
            uint64_t attach[ROUNDS], detach[ROUNDS];
            /* EwokOS fork clones heap/stack, not the SHM window. */
            unsigned bad = 0;
            for (unsigned n = 0; n < ROUNDS; n++) {
                uint64_t begin = now_ns();
                volatile uint32_t *p = shmat(ids[w], 0, 0);
                attach[n] = now_ns() - begin;
                if (!p || p == (void*)-1) exit(4);
                for (unsigned off = 0; off < size/4; off += page/4) {
                    bad += p[off] != pattern(w, n, off);
                    p[off] = pattern(w, n+1, off);
                }
                bad += p[size/4-1] != pattern(w, n, size/4-1);
                p[size/4-1] = pattern(w, n+1, size/4-1);
                if (n == ROUNDS-1) p[1] = bad ? 2 : 1;
                begin = now_ns();
                int rc = shmdt((void*)p);
                detach[n] = now_ns() - begin;
                if (rc != 0) exit(5);
            }
            report("attach", size, contig, workers, w, attach);
            report("detach", size, contig, workers, w, detach);
            klog("shm-map verify bytes=%u contig=%u workers=%u worker=%u errors=%u\n",
                 size, contig, workers, w, bad);
            exit(bad ? 6 : 0);
        }
        children[started++] = pid;
    }
    for (unsigned w = 0; w < started; w++) {
        /* This OS reaps exited children eagerly; use its wait primitive
         * and verify shared results instead of POSIX exit status. */
        ewok_waitpid(children[w]);
        errors += keepers[w][1] != 1;
        for (unsigned off = 0; off < size/4; off += page/4)
            errors += keepers[w][off] != pattern(w, ROUNDS, off);
        errors += keepers[w][size/4-1] != pattern(w, ROUNDS, size/4-1);
    }
    for (unsigned w = 0; w < created; w++) {
        errors += shmdt((void*)keepers[w]) != 0;
        /* Last detach normally frees the segment already. */
        shmctl(ids[w], IPC_RMID, 0);
    }
    klog("shm-map case bytes=%u contig=%u workers=%u errors=%u\n",
         size, contig, workers, errors);
    return errors ? 1 : 0;
}

int main(void) {
    sys_info_t info;
    if (sys_get_sys_info(&info) != 0 || !info.page_size) return 1;
    const unsigned sizes[] = {65536u, 8388608u, 33554432u};
    unsigned errors = 0;
    klog("shm-map BEGIN page=%u rounds=%u\n", info.page_size, ROUNDS);
    for (unsigned c = 0; c < 2; c++)
        for (unsigned s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++)
            for (unsigned w = 1; w <= WORKERS; w *= WORKERS)
                errors += run(sizes[s], info.page_size, c, w);
    klog("shm-map DONE errors=%u\n", errors);
    return errors ? 1 : 0;
}
