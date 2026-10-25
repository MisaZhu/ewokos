/* End-to-end IPC through the running g2dd. One canvas fits the mapping
 * cache; five 4K canvases exceed its 128MB budget. No direct GPU ownership. */
#include <g2dclient/g2dclient.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define WIDTH 3840u
#define HEIGHT 2160u
#define PIXELS (WIDTH * HEIGHT)
#define BYTES (PIXELS * 4u + 256u)
#define GUARD 0xc7142af9u
#define MAX_CANVASES 5u
#define SAMPLES 100u

static uint64_t now_ns(void) {
    uint64_t ns = 0;
    if (kernel_tic_nsec(&ns) != 0) exit(2);
    return ns;
}

int main(int argc, char **argv) {
    unsigned count = argc > 1 ? (unsigned)atoi(argv[1]) : MAX_CANVASES;
    if (!count || count > MAX_CANVASES) return 1;
    g2d_canvas_t canvas[MAX_CANVASES];
    uint32_t *pixels[MAX_CANVASES];
    unsigned created = 0, errors = 0;
    uint64_t samples[SAMPLES], total = 0;
    klog("g2d-ipc BEGIN canvases=%u bytes=%u n=%u\n", count, BYTES, SAMPLES);
    for (unsigned i = 0; i < count; i++) {
        int id;
        ewokos_addr_t phy;
        if (g2d_shm_alloc_phy(BYTES, &id, &pixels[i], &phy) != 0) {
            errors++; goto cleanup;
        }
        created++;
        canvas[i] = g2d_canvas(id, BYTES, WIDTH, HEIGHT, 1);
        canvas[i].phy = phy;
        for (unsigned j = 0; j < 64; j++) pixels[i][PIXELS+j] = GUARD;
    }
    /* Warm each canvas once; exclude allocation and first-use costs. */
    for (unsigned n = 0; n < count + SAMPLES; n++) {
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
        }
    }
    for (unsigned i = 0; i < count; i++) {
        for (unsigned j = 0; j < PIXELS; j++)
            errors += pixels[i][j] != 0xff306090u+i;
        for (unsigned j = 0; j < 64; j++)
            errors += pixels[i][PIXELS+j] != GUARD;
    }
    for (unsigned i = 1; i < SAMPLES; i++)
        for (unsigned j = i; j && samples[j] < samples[j-1]; j--) {
            uint64_t tmp = samples[j]; samples[j] = samples[j-1]; samples[j-1] = tmp;
        }
    klog("g2d-ipc RESULT canvases=%u n=%u mean_ns=%llu p50_ns=%llu p95_ns=%llu max_ns=%llu errors=%u\n",
         count, SAMPLES, (unsigned long long)(total/SAMPLES),
         (unsigned long long)samples[SAMPLES/2],
         (unsigned long long)samples[SAMPLES*95/100],
         (unsigned long long)samples[SAMPLES-1], errors);
cleanup:
    for (unsigned i = 0; i < created; i++) g2d_shm_free(pixels[i]);
    klog("g2d-ipc DONE errors=%u\n", errors);
    return errors ? 1 : 0;
}
