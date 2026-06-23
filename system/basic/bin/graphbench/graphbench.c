#include <graph/graph.h>
#include <graph/bsp_graph.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

typedef struct {
    const char* name;
    int32_t w;
    int32_t h;
    int use_alpha;
    uint8_t alpha;
    int edge_pattern;
} bench_case_t;

static uint64_t now_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static uint32_t graph_checksum(const graph_t* g) {
    uint32_t hash = 2166136261u;
    int32_t total = g->w * g->h;

    for (int32_t i = 0; i < total; ++i) {
        hash ^= g->buffer[i];
        hash *= 16777619u;
    }
    return hash;
}

static void fill_src_pattern(graph_t* g, int edge_pattern) {
    for (int32_t y = 0; y < g->h; ++y) {
        uint32_t* row = g->buffer + y * g->w;
        for (int32_t x = 0; x < g->w; ++x) {
            uint8_t r = (uint8_t)((x * 13 + y * 3) & 0xff);
            uint8_t gch = (uint8_t)((x * 5 + y * 11) & 0xff);
            uint8_t b = (uint8_t)((x * 7 + y * 17) & 0xff);
            uint8_t a;

            if (!edge_pattern) {
                a = (uint8_t)(((x * 19 + y * 23) & 0x7f) + 0x80);
            } else {
                int tile = ((x >> 4) + (y >> 4)) & 0x3;
                if (tile == 0)
                    a = 0x00;
                else if (tile == 1)
                    a = 0xff;
                else if (tile == 2)
                    a = (uint8_t)((x * 9 + y * 15) & 0xff);
                else
                    a = (uint8_t)(((x ^ y) * 21) & 0xff);
            }

            row[x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) |
                ((uint32_t)gch << 8) | b;
        }
    }
}

static void fill_dst_pattern(graph_t* g) {
    for (int32_t y = 0; y < g->h; ++y) {
        uint32_t* row = g->buffer + y * g->w;
        for (int32_t x = 0; x < g->w; ++x) {
            uint8_t a = (uint8_t)(0x40 + ((x * 7 + y * 9) & 0x7f));
            uint8_t r = (uint8_t)((x * 3 + y * 5) & 0xff);
            uint8_t gch = (uint8_t)((x * 17 + y * 13) & 0xff);
            uint8_t b = (uint8_t)((x * 29 + y * 31) & 0xff);
            row[x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) |
                ((uint32_t)gch << 8) | b;
        }
    }
}

static void reset_dst(graph_t* dst, const graph_t* init) {
    memcpy(dst->buffer, init->buffer, (size_t)init->w * init->h * sizeof(uint32_t));
}

static void run_case(const bench_case_t* c, int iterations) {
    graph_t* src = graph_new(NULL, c->w, c->h);
    graph_t* dst_init = graph_new(NULL, c->w, c->h);
    graph_t* dst_bsp = graph_new(NULL, c->w, c->h);
    graph_t* dst_cpu = graph_new(NULL, c->w, c->h);
    uint64_t start_usec;
    uint64_t bsp_total;
    uint64_t cpu_total;
    uint32_t bsp_hash;
    uint32_t cpu_hash;
    double bsp_us_per_op;
    double cpu_us_per_op;
    double speedup;

    if (src == NULL || dst_init == NULL || dst_bsp == NULL || dst_cpu == NULL) {
        printf("%-18s alloc failed\n", c->name);
        graph_free(src);
        graph_free(dst_init);
        graph_free(dst_bsp);
        graph_free(dst_cpu);
        return;
    }

    fill_src_pattern(src, c->edge_pattern);
    fill_dst_pattern(dst_init);
    reset_dst(dst_bsp, dst_init);
    reset_dst(dst_cpu, dst_init);

    for (int i = 0; i < 16; ++i) {
        if (c->use_alpha)
            graph_blt_alpha_bsp(src, 0, 0, c->w, c->h, dst_bsp, 0, 0, c->w, c->h, c->alpha);
        else
            graph_blt_bsp(src, 0, 0, c->w, c->h, dst_bsp, 0, 0, c->w, c->h);
    }

    reset_dst(dst_bsp, dst_init);
    start_usec = now_usec();
    for (int i = 0; i < iterations; ++i) {
        if (c->use_alpha)
            graph_blt_alpha_bsp(src, 0, 0, c->w, c->h, dst_bsp, 0, 0, c->w, c->h, c->alpha);
        else
            graph_blt_bsp(src, 0, 0, c->w, c->h, dst_bsp, 0, 0, c->w, c->h);
    }
    bsp_total = now_usec() - start_usec;
    bsp_hash = graph_checksum(dst_bsp);

    reset_dst(dst_cpu, dst_init);
    start_usec = now_usec();
    for (int i = 0; i < iterations; ++i) {
        if (c->use_alpha)
            graph_blt_alpha_cpu(src, 0, 0, c->w, c->h, dst_cpu, 0, 0, c->w, c->h, c->alpha);
        else
            graph_blt_cpu(src, 0, 0, c->w, c->h, dst_cpu, 0, 0, c->w, c->h);
    }
    cpu_total = now_usec() - start_usec;
    cpu_hash = graph_checksum(dst_cpu);

    bsp_us_per_op = (double)bsp_total / (double)iterations;
    cpu_us_per_op = (double)cpu_total / (double)iterations;
    speedup = (bsp_us_per_op > 0.0) ? (cpu_us_per_op / bsp_us_per_op) : 0.0;

    printf("%-18s %4dx%-4d bsp=%9.2fus  cpu=%9.2fus  speedup=%5.2fx  %s\n",
        c->name, c->w, c->h, bsp_us_per_op, cpu_us_per_op, speedup,
        (bsp_hash == cpu_hash) ? "OK" : "DIFF");

    graph_free(src);
    graph_free(dst_init);
    graph_free(dst_bsp);
    graph_free(dst_cpu);
}

int main(int argc, char* argv[]) {
    int iterations = 300;
    bench_case_t cases[] = {
        {"blt-copy-64", 64, 64, 0, 0x00, 0},
        {"blt-copy-128", 128, 128, 0, 0x00, 0},
        {"blt-copy-192", 192, 192, 0, 0x00, 0},
        {"blt-copy-256", 256, 256, 0, 0x00, 0},
        {"blt-copy-512", 512, 512, 0, 0x00, 0},
        {"alpha255-128", 128, 128, 1, 0xff, 0},
        {"alpha128-128", 128, 128, 1, 0x80, 0},
        {"alpha-edge-128", 128, 128, 1, 0xff, 1},
    };
    int case_count = (int)(sizeof(cases) / sizeof(cases[0]));

    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0)
            iterations = 300;
    }

    printf("graphbench iterations=%d\n", iterations);
    printf("compare current BSP path against CPU baseline\n");
    for (int i = 0; i < case_count; ++i)
        run_case(&cases[i], iterations);

    return 0;
}
