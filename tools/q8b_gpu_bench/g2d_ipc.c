/* End-to-end IPC through the running g2dd. One canvas fits the mapping
 * cache; five 4K canvases exceed its 128MB budget. No direct GPU ownership. */
#include <g2dclient/g2dclient.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <procinfo.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 3840u
#define HEIGHT 2160u
#define PIXELS (WIDTH * HEIGHT)
#define BYTES (PIXELS * 4u + 256u)
#define GUARD 0xc7142af9u
#define MAX_CANVASES 5u
#define DEFAULT_SAMPLES 100u

static uint64_t now_ns(void) {
    uint64_t ns = 0;
    if (kernel_tic_nsec(&ns) != 0) exit(2);
    return ns;
}

/* Same process+thread accounting as ps; one million is one full CPU core.
 * Sample outside timed requests and discard the first two-second window. */
static uint64_t daemon_cpu(int pid) {
    int n = syscall0(SYS_GET_PROCS_NUM);
    if (n <= 0) return UINT64_MAX;
    procinfo_t *procs = malloc((unsigned)n * sizeof(*procs));
    if (!procs) return UINT64_MAX;
    int got = syscall2(SYS_GET_PROCS, (ewokos_addr_t)n, (ewokos_addr_t)procs);
    uint64_t cpu = 0;
    int found = 0;
    for (int i = 0; i < got; i++) {
        if (procs[i].pid == pid ||
                (procs[i].type == TASK_TYPE_THREAD && procs[i].father_pid == pid)) {
            cpu += procs[i].run_usec;
            found = 1;
        }
    }
    free(procs);
    return found ? cpu : UINT64_MAX;
}

int main(int argc, char **argv) {
    unsigned count = argc > 1 ? (unsigned)atoi(argv[1]) : MAX_CANVASES;
    unsigned rounds = argc > 2 ? (unsigned)atoi(argv[2]) : DEFAULT_SAMPLES;
    unsigned rate = argc > 3 ? (unsigned)atoi(argv[3]) : 0;
    if (!count || count > MAX_CANVASES || !rounds || rounds > 16384u || rate > 1000u) return 1;
    g2d_canvas_t canvas[MAX_CANVASES];
    uint32_t *pixels[MAX_CANVASES];
    unsigned created = 0, errors = 0;
    uint64_t *samples = malloc(rounds * sizeof(*samples)), total = 0;
    uint64_t measured_begin = 0, elapsed = 0, last_cpu = 0, cpu_total = 0;
    unsigned cpu_samples = 0;
    int daemon = dev_get_pid("/dev/g2d");
    if (!samples) return 1;
    klog("g2d-ipc BEGIN canvases=%u bytes=%u n=%u rate=%u\n", count, BYTES, rounds, rate);
    for (unsigned i = 0; i < count; i++) {
        int id;
        ewokos_addr_t phy;
        if (g2d_shm_alloc_phy(BYTES, &id, &pixels[i], &phy) != 0) {
            klog("g2d-ipc allocation failed canvas=%u\n", i);
            errors++; goto cleanup;
        }
        created++;
        canvas[i] = g2d_canvas(id, BYTES, WIDTH, HEIGHT, 1);
        canvas[i].phy = phy;
        for (unsigned j = 0; j < 64; j++) pixels[i][PIXELS+j] = GUARD;
    }
    /* Warm each canvas once; exclude allocation and first-use costs. */
    for (unsigned n = 0; n < count + rounds; n++) {
        if (n == count) measured_begin = last_cpu = now_ns();
        if (rate && n >= count) {
            uint64_t target = measured_begin + (uint64_t)(n-count)*1000000000u/rate;
            uint64_t current;
            while ((current = now_ns()) < target)
                usleep((unsigned)((target-current+999u)/1000u));
        }
        unsigned i = n % count;
        g2d_fill_req_t req;
        g2d_rect_t rect = {0, 0, WIDTH, HEIGHT};
        g2d_fill_req_init(&req, canvas[i], rect, 0xff306090u + i);
        uint64_t begin = now_ns();
        int rc = g2d_fill_rect(&req);
        uint64_t duration = now_ns() - begin;
        if (rc != G2D_OK) {
            klog("g2d-ipc request n=%u rc=%d\n", n, rc);
            /* Retain every buffer if a submitted job can still reference it. */
            if (rc == G2D_ERR_TIMEOUT || rc == G2D_ERR_MMU_FAULT)
                for (;;) usleep(1000000);
            errors++; goto cleanup;
        }
        if (n >= count) {
            samples[n-count] = duration;
            total += duration;
            uint64_t current = now_ns();
            elapsed = current - measured_begin;
            if (elapsed >= 2000000000u && current-last_cpu >= 500000000u) {
                uint64_t cpu = daemon_cpu(daemon);
                if (cpu != UINT64_MAX) { cpu_total += cpu; cpu_samples++; }
                last_cpu = current;
            }
        }
    }
    for (unsigned i = 0; i < count; i++) {
        for (unsigned j = 0; j < PIXELS; j++)
            errors += pixels[i][j] != 0xff306090u+i;
        for (unsigned j = 0; j < 64; j++)
            errors += pixels[i][PIXELS+j] != GUARD;
    }
    for (unsigned i = 1; i < rounds; i++)
        for (unsigned j = i; j && samples[j] < samples[j-1]; j--) {
            uint64_t tmp = samples[j]; samples[j] = samples[j-1]; samples[j-1] = tmp;
        }
    klog("g2d-ipc RESULT canvases=%u n=%u mean_ns=%llu p50_ns=%llu p95_ns=%llu max_ns=%llu errors=%u\n",
         count, rounds, (unsigned long long)(total/rounds),
         (unsigned long long)samples[rounds/2],
         (unsigned long long)samples[rounds*95/100],
         (unsigned long long)samples[rounds-1], errors);
    klog("g2d-ipc LOAD elapsed_ns=%llu cpu_samples=%u cpu_ppm=%llu est_cpu_us_per_req=%llu\n",
         (unsigned long long)elapsed, cpu_samples,
         (unsigned long long)(cpu_samples ? cpu_total/cpu_samples : 0),
         (unsigned long long)(cpu_samples ? (cpu_total/cpu_samples)*elapsed/rounds/1000000000u : 0));
cleanup:
    for (unsigned i = 0; i < created; i++) g2d_shm_free(pixels[i]);
    free(samples);
    klog("g2d-ipc DONE errors=%u\n", errors);
    return errors ? 1 : 0;
}
