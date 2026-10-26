/* Exclusive, offscreen hardware test. Run once per boot, without g2dd. */
#include <adreno/adreno_drv.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/shm.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PIXELS (3840u * 2160u)
#define GUARD_WORDS 64u
#define GUARD 0xa17e39c5u
static uint32_t *source, *dest;
static uint64_t source_pa, dest_pa;
static unsigned failures;
static int texture_program = -1;
static const char *names[] = {"fill", "copy", "fill-alpha", "blend", "scale", "rotate90", "texture", "mixed"};

static uint64_t now_us(void) {
    uint64_t ns;
    kernel_tic_nsec(&ns);
    return ns / 1000u;
}

static void park(void) {
    /* Keep ring/GMU allocations alive: this driver has no shutdown protocol. */
    for (;;) usleep(1000000);
}

static void require_gpu(int rc, const char *op) {
    if (rc == ADRENO_OK) return;
    klog("gpu-bench FAIL op=%s rc=%d fence=%u wedged=%d; retaining all buffers\n",
         op, rc, adreno_dev()->fence, adreno_wedged());
    park();
}

static uint32_t pattern(unsigned x, unsigned y) {
    return (((x+y)%254u+1u)<<24) | ((x*31u+y*17u)&0xffffffu);
}

static uint32_t blend(uint32_t src, uint32_t dst, unsigned a) {
    unsigned da=dst>>24;
    if (!a) return dst;
    if (!da) return (src&0xffffffu) | (a<<24);
    uint32_t out=(da+(255u-da)*a/255u)<<24;
    for (unsigned shift=0;shift<24;shift+=8)
        out |= ((((src>>shift)&255u)*a+((dst>>shift)&255u)*(255u-a))/255u)<<shift;
    return out;
}

static void guards(unsigned n) {
    for (unsigned i=0;i<GUARD_WORDS;i++) dest[n+i]=GUARD;
}

static void check_guards(unsigned n, const char *op) {
    for (unsigned i=0;i<GUARD_WORDS;i++) if (dest[n+i]!=GUARD) {
        klog("gpu-bench FAIL guard op=%s word=%u got=%08x\n",op,i,dest[n+i]);
        park();
    }
}

static int render(unsigned op, unsigned w, unsigned h, unsigned seed) {
    switch (op) {
    case 0: return adreno_2d_fill(dest_pa,w,h,w*4u,0,0,w,h,0xff123456u,0);
    case 1: return adreno_2d_copy(source_pa,w,h,w*4u,0,0,w,h,
                                dest_pa,w,h,w*4u,0,0,w,h,0);
    case 2: return adreno_compute_fill_alpha(dest_pa,w,h,0,0,w,h,0x809abcdeu);
    case 3: return adreno_compute_blend(source_pa,w,h,0,0,w,h,dest_pa,w,h,0,0,w,h,173);
    case 4: return adreno_compute_scale(source_pa,w,h,0,0,w,h,dest_pa,w,h,0,0,w,h,1);
    case 5: return adreno_compute_rotate(source_pa,w,h,dest_pa,h,w,90,0,16384);
    case 6: {
        uint32_t u[4]={w,h,seed,0};
        adreno_3d_tex_t tex={0};
        tex.phy=source_pa; tex.w=16; tex.h=16; tex.stride=64; tex.depth=1;
        return adreno_3d_draw(dest_pa,w,h,w*4u,texture_program,NULL,&tex,1,u,4);
    }
    case 7:
        for (unsigned i=0;i<3;i++) {
            unsigned sub=i==0?0:i==1?1:3;
            int rc=render(sub,w,h,seed);
            if (rc) return rc;
        }
        return 0;
    }
    return -1;
}

static void correctness(unsigned op, unsigned w, unsigned h) {
    unsigned n=w*h;
    for (unsigned y=0;y<h;y++) for (unsigned x=0;x<w;x++) source[y*w+x]=pattern(x,y);
    if (op==6) for (unsigned i=0;i<256;i++) source[i]=0xff563412u;
    for (unsigned i=0;i<n;i++) dest[i]=0xff243648u;
    guards(n);
    require_gpu(render(op,w,h,7),names[op]);
    unsigned bad=0;
    for (unsigned i=0;i<n;i++) {
        uint32_t expected;
        unsigned x=i%w,y=i/w;
        switch(op) {
        case 0: expected=0xff123456u; break;
        case 1: case 4: expected=pattern(x,y); break;
        case 2: expected=blend(0x809abcdeu,0xff243648u,128); break;
        case 3: expected=blend(pattern(x,y),0xff243648u,((pattern(x,y)>>24)*173u)>>8); break;
        case 5: expected=pattern(i/h,h-1u-i%h); break;
        case 6: expected=0xff563412u^7u; break;
        default: expected=blend(pattern(x,y),pattern(x,y),((pattern(x,y)>>24)*173u)>>8); break;
        }
        if(dest[i]!=expected) {
            if (!bad) klog("gpu-bench mismatch op=%s %ux%u at=%u got=%08x want=%08x\n",
                           names[op],w,h,i,dest[i],expected);
            bad++;
        }
    }
    check_guards(n,names[op]);
    if (bad) failures++;
    klog("gpu-bench pixels op=%s %ux%u checked=%u bad=%u\n",names[op],w,h,n,bad);
}

static int compare(const void *a, const void *b) {
    uint32_t x=*(const uint32_t*)a,y=*(const uint32_t*)b;
    return (x>y)-(x<y);
}

static void benchmark(unsigned op,unsigned w,unsigned h) {
    uint32_t samples[4096];
    unsigned count=0;
    uint64_t begin=now_us(),elapsed;
    do {
        uint64_t start=now_us();
        require_gpu(render(op,w,h,count),names[op]);
        samples[count++]=(uint32_t)(now_us()-start);
        elapsed=now_us()-begin;
    } while (elapsed<750000u && count<4096);
    qsort(samples,count,sizeof(*samples),compare);
    check_guards(w*h,names[op]);
    klog("gpu-bench perf op=%s %ux%u n=%u mean=%llu min=%u p50=%u p95=%u max=%u us fence=%u\n",
         names[op],w,h,count,(unsigned long long)(elapsed/count),samples[0],samples[count/2],
         samples[(count-1)*95/100],samples[count-1],adreno_dev()->fence);
}

static void allocate(uint32_t **va,uint64_t *pa) {
    static unsigned sequence;
    /* IPC_PRIVATE on older kernels drops the allocation flags. */
    key_t key=0x47500000u+((unsigned)getpid()<<8)+sequence++;
    int id=shmget(key,(MAX_PIXELS+GUARD_WORDS)*4u,IPC_CREAT|IPC_EXCL|IPC_CONTIG|0600);
    if (id<0 || (*va=shmat(id,NULL,0))==(void*)-1 ||
        !(*pa=shm_contig_phy_addr(id,(ewokos_addr_t)*va))) {
        klog("gpu-bench FAIL contiguous buffer allocation\n"); park();
    }
    shmctl(id,IPC_RMID,NULL); /* attached until this process is deliberately rebooted */
}

static void load_texture_program(const char *path) {
    FILE *f=fopen(path,"rb");
    if (!f) { klog("gpu-bench FAIL open %s\n",path); park(); }
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f);
    void *data=n>0 && n<1024*1024?malloc(n):NULL;
    if (!data || fread(data,1,n,f)!=(size_t)n) { klog("gpu-bench FAIL shader read\n"); park(); }
    fclose(f);
    texture_program=adreno_3d_program_load(data,n);
    free(data);
    if (texture_program<0) { klog("gpu-bench FAIL program load=%d\n",texture_program); park(); }
}

static void descriptor_reuse(const char *path) {
    adreno_3d_tex_t tex={0};
    tex.phy=source_pa; tex.w=16; tex.h=16; tex.stride=64; tex.depth=1;
    for (unsigned i=0;i<256;i++) source[i]=0xff563412u;
    /* Alternate full and short uniform uploads on one retained descriptor,
       then destroy/reload the program so its pool is released and recreated. */
    for (unsigned cycle=0;cycle<32;cycle++) {
        for (unsigned j=0;j<8;j++) {
            uint32_t u[4]={64,64,0xabcdefu+cycle,0};
            unsigned count=(j&1u)?2:4;
            uint32_t expected=0xff563412u^(count==4?u[2]:0u);
            guards(64*64);
            require_gpu(adreno_3d_draw(dest_pa,64,64,256,texture_program,
                                      NULL,&tex,1,u,count),"uniform-reuse");
            for (unsigned i=0;i<64*64;i++) if (dest[i]!=expected) {
                klog("gpu-bench FAIL uniform-reuse cycle=%u short=%u at=%u got=%08x want=%08x\n",
                     cycle,count==2,i,dest[i],expected);
                park();
            }
            check_guards(64*64,"uniform-reuse");
        }
        require_gpu(adreno_3d_program_free(texture_program),"program-free");
        load_texture_program(path);
    }
    klog("gpu-bench descriptor-reuse cycles=32 draws=256 short-uniforms=128 bad=0\n");
}

int main(int argc,char **argv) {
    unsigned stress_seconds=argc>1?(unsigned)atoi(argv[1]):120;
    if (stress_seconds>3600) return 1;
    setenv("ADRENO_ENABLE","1");
    klog("gpu-bench begin t=%llu stress=%u pid=%d\n",(unsigned long long)now_us(),stress_seconds,getpid());
    if (!adreno_2d_ready() || !adreno_3d_ready()) { klog("gpu-bench FAIL GPU init\n"); park(); }
    allocate(&source,&source_pa); allocate(&dest,&dest_pa);
    const char *texture_path=argc>2?argv[2]:"/gpu-bench/texture.bin";
    load_texture_program(texture_path);
    descriptor_reuse(texture_path);
    uint64_t start=now_us();
    for (unsigned i=0;i<512;i++) {
        adreno_bo_t b={0};
        if (adreno_bo_alloc(&b,256)) { klog("gpu-bench FAIL alloc benchmark\n"); park(); }
        adreno_bo_free(&b);
    }
    klog("gpu-bench dma-cycle n=512 mean=%llu us\n",(unsigned long long)((now_us()-start)/512));
    const unsigned sizes[][2]={{64,64},{640,480},{801,603},{1920,1080},{3840,2160}};
    unsigned passes=argc>3 && strcmp(argv[3],"ab")==0 ? 2u : 1u;
    for (unsigned pass=0;pass<passes;pass++) {
        char *cmd[]={"submit","wait",passes==2 && pass==0?"0":"1"};
        char status[256];
        if (adreno_ring_cmd(3,cmd,status,sizeof(status)) != 0) {
            klog("gpu-bench FAIL wait policy\n"); park();
        }
        klog("gpu-bench mode adaptive=%s\n",cmd[2]);
        for (unsigned s=0;s<sizeof(sizes)/sizeof(sizes[0]);s++) {
            unsigned w=sizes[s][0],h=sizes[s][1];
            for (unsigned op=0;op<8;op++) {
                if ((w&15u) && (op==0 || op==1 || op==7)) continue;
                correctness(op,w,h);
                benchmark(op,w,h);
            }
        }
    }
    /* Repeated A2D/compute and descriptor-size transitions, with idle gaps. */
    start=now_us();
    unsigned frames=0;
    while (now_us()-start<(uint64_t)stress_seconds*1000000u) {
        unsigned w=(frames&1u)?1920:640,h=(frames&1u)?1080:480;
        guards(w*h);
        for (unsigned op=0;op<8;op++) require_gpu(render(op,w,h,frames),names[op]);
        check_guards(w*h,"stress");
        frames++;
        if (frames%500u==0) {
            klog("gpu-bench stress seconds=%llu frames=%u fence=%u\n",
                 (unsigned long long)((now_us()-start)/1000000u),frames,adreno_dev()->fence);
            usleep(20000);
        }
    }
    klog("gpu-bench DONE failures=%u stress_frames=%u fence=%u wedged=%d t=%llu\n",
         failures,frames,adreno_dev()->fence,adreno_wedged(),(unsigned long long)now_us());
    park();
}
