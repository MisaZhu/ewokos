/* Exclusive GPU owner. Reset before/after; never run beside g2dd. */
#include <adreno/adreno_drv.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/shm.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "geometry.h"

#define MAX_PIXELS (1920u*1080u)
#define MAX_VERTS 20478u
#define GUARD 0xa17e39c5u
static uint32_t *fb,*depth,*texture,*saved_fb,*saved_z;
static float *pos,*col;
static uint64_t fb_pa,z_pa,pos_pa,col_pa,tex_pa;
static uint64_t checked;
static unsigned cases;
static struct geom_meta meta;
static uint32_t uniforms[256];
static adreno_3d_geom_t geom;
static int program;
static int reference_program=-1;

static void park(void) { for (;;) usleep(1000000); }
static void need(int ok,const char *why) {
    if (!ok) { klog("geom-bench FAIL %s case=%u fence=%u wedged=%d\n",why,cases,adreno_dev()->fence,adreno_wedged()); park(); }
}
static uint64_t ns(void) { uint64_t v; kernel_tic_nsec(&v); return v; }
static void alloc(void **va,uint64_t *pa,unsigned bytes) {
    static unsigned seq;
    int id=shmget(0x47300000u+((unsigned)getpid()<<8)+seq++,bytes,IPC_CREAT|IPC_EXCL|IPC_CONTIG|0600);
    need(id>=0,"shmget"); *va=shmat(id,NULL,0); need(*va!=(void*)-1,"shmat");
    *pa=shm_contig_phy_addr(id,(ewokos_addr_t)*va); need(*pa!=0,"physical"); shmctl(id,IPC_RMID,NULL);
}
static void read_file(const char *path,void **p,unsigned *bytes) {
    FILE *f=fopen(path,"rb"); need(f!=NULL,"open");
    fseek(f,0,SEEK_END); long n=ftell(f); rewind(f); need(n>0 && n<1024*1024,"file size");
    *p=malloc(n); need(*p!=NULL,"malloc"); need(fread(*p,1,n,f)==(size_t)n,"read"); fclose(f); *bytes=n;
}
static void load(const char *dir,const char *name) {
    char path[512]; void *p; unsigned n;
    snprintf(path,sizeof(path),"%s/geom-%s.meta",dir,!strcmp(name,"invalid")?"color":name); read_file(path,&p,&n);
    need(n==sizeof(meta),"metadata size"); memcpy(&meta,p,n); free(p); need(meta.uniforms<=256,"uniform size");
    snprintf(path,sizeof(path),"%s/geom-%s.bin",dir,name); read_file(path,&p,&n);
    program=adreno_3d_program_load(p,n); free(p); need(program>=0,"program load");
}
static void puti(int off,int value) { if (off>=0) uniforms[off]=value; }
static void putf(int off,float value) { if (off>=0) memcpy(&uniforms[off],&value,4); }
static void scene(unsigned w,unsigned h,unsigned count,unsigned kind,unsigned seed) {
    memset(uniforms,0,sizeof(uniforms));
    for (unsigned i=0;i<4;i++) putf(meta.mvp+i*5,1.0f);
    putf(meta.viewport_x,w); putf(meta.viewport_x+1,h); puti(meta.vertices,count*3);
    putf(meta.viewport_r,w); putf(meta.viewport_r+1,h); puti(meta.stride,w);
    puti(meta.first,0); puti(meta.count,count); puti(meta.depth_write,1);
    puti(meta.tex_w,16); puti(meta.tex_h,16);
    unsigned nx=1; while(nx*nx<count) nx++;
    for(unsigned t=0;t<count;t++) {
        unsigned cx=t%nx,cy=t/nx;
        float x0=(cx+0.125f)*w/nx,y0=(cy+0.125f)*h/nx;
        float x1=(cx+0.875f)*w/nx,y1=(cy+0.875f)*h/nx;
        float xy[6]={x0,y0,x1,y0,x0,y1};
        float z=((seed+t)%5)*0.125f-0.25f;
        unsigned rgb=0x204060u+((seed+t)*0x030507u)%0x9f9f9fu;
        for(unsigned v=0;v<3;v++) {
            unsigned i=t*3+v, stride=kind==1?3:6;
            pos[i*stride]=xy[v*2]*2.0f/w-1.0f;
            pos[i*stride+1]=1.0f-xy[v*2+1]*2.0f/h; pos[i*stride+2]=z;
            if(kind==2) { pos[i*6+3]=0.28125f; pos[i*6+4]=0.71875f; pos[i*6+5]=0; }
            col[i*3]=((rgb>>16)&255u)/255.0f;
            col[i*3+1]=((rgb>>8)&255u)/255.0f; col[i*3+2]=(rgb&255u)/255.0f;
        }
    }
    memset(&geom,0,sizeof(geom)); geom.mode=4; geom.nverts=count*3;
    geom.pos_phy=pos_pa; geom.col_phy=col_pa; geom.z_phy=z_pa; geom.z_stride=w*4;
    geom.ctex_phy=tex_pa; geom.ctex_w=16; geom.ctex_h=16; geom.ctex_stride=64;
}
static void clear(unsigned n) {
    for(unsigned i=0;i<n;i++) { fb[i]=0xff102030u; depth[i]=0xffffffffu; }
    for(unsigned i=0;i<64;i++) { fb[n+i]=GUARD; depth[n+i]=GUARD; }
}
static void draw(unsigned w,unsigned h) {
    need(adreno_3d_draw(fb_pa,w,h,w*4,program,&geom,NULL,0,uniforms,meta.uniforms)==0,"draw");
}
static void reference_draw(unsigned w,unsigned h) {
    int current=program;
    if(reference_program>=0) program=reference_program;
    draw(w,h); program=current;
}
static void guards(unsigned n) {
    for(unsigned i=0;i<64;i++) need(fb[n+i]==GUARD && depth[n+i]==GUARD,"guard");
}
/* Independent analytic oracle for a single constant-colour triangle. Exclude
 * only edge roundoff; A/B comparison below still checks every edge pixel. */
static void oracle(unsigned w,unsigned h,unsigned kind,unsigned seed) {
    float x0=0.125f*w,y0=0.125f*h,x1=0.875f*w,y1=0.875f*h;
    uint32_t rgb=0x204060u+(seed*0x030507u)%0x9f9f9fu;
    uint32_t color=kind==1?(0xff000000u|rgb):texture[4*16+4];
    uint32_t z=(uint32_t)((((seed%5)*0.125f-0.25f)*0.5f+0.5f)*16777215.0f);
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
        float a=(x+0.5f-x0)/(x1-x0),b=(y+0.5f-y0)/(y1-y0);
        if(fabsf(a)<0.0001f || fabsf(b)<0.0001f || fabsf(a+b-1)<0.0001f) continue;
        int inside=a>0 && b>0 && a+b<1;
        unsigned i=y*w+x;
        if(!inside) need(fb[i]==0xff102030u && depth[i]==0xffffffffu,"oracle background");
        else {
            need((fb[i]>>24)==255,"oracle alpha");
            for(unsigned shift=0;shift<24;shift+=8)
                need(abs((int)((fb[i]>>shift)&255)-(int)((color>>shift)&255))<=1,"oracle color");
            need(abs((int)depth[i]-(int)z)<=2,"oracle depth");
        }
    }
}
static void correctness(unsigned w,unsigned h,unsigned count,unsigned kind,unsigned seed) {
    scene(w,h,count,kind,seed); unsigned n=w*h;
    adreno_3d_geom_batch(0); clear(n); reference_draw(w,h); guards(n);
    memcpy(saved_fb,fb,n*4); memcpy(saved_z,depth,n*4);
    adreno_3d_geom_batch(1); clear(n); draw(w,h); guards(n);
    need(!memcmp(saved_fb,fb,n*4) && !memcmp(saved_z,depth,n*4),"A/B pixels/depth");
    if(count==1) oracle(w,h,kind,seed);
    checked+=n; cases++;
    /* Reusing both descriptor slots with a new transform must be visible. */
    putf(meta.mvp+12,0.0625f); clear(n); draw(w,h); guards(n);
    adreno_3d_geom_batch(0); memcpy(saved_fb,fb,n*4); memcpy(saved_z,depth,n*4);
    clear(n); reference_draw(w,h); guards(n);
    need(!memcmp(saved_fb,fb,n*4) && !memcmp(saved_z,depth,n*4),"reuse transform");
    checked+=n; cases++;
}
static void perf(unsigned w,unsigned h,unsigned count,unsigned kind) {
    scene(w,h,count,kind,3); unsigned repeats=w*h>1000000?8:32;
    char status[1024];
    char *enable[]={"submit","gpu","1"},*disable[]={"submit","gpu","0"};
    char *reset[]={"submit","reset"},*query[]={"submit"};
    need(!adreno_ring_cmd(3,enable,status,sizeof(status)),"profile enable");
    for(unsigned pass=0;pass<4;pass++) {
        int batch=(pass==1 || pass==2); adreno_3d_geom_batch(batch);
        putf(meta.mvp+14,0.0f); clear(w*h); draw(w,h);
        need(!adreno_ring_cmd(2,reset,status,sizeof(status)),"profile reset");
        uint32_t fence=adreno_dev()->fence;
        uint64_t begin=ns();
        for(unsigned i=0;i<repeats;i++) { putf(meta.mvp+14,-(float)(i+1)/512.0f); draw(w,h); }
        uint64_t elapsed=ns()-begin;
        guards(w*h);
        need(adreno_dev()->fence-fence==repeats*(batch?1u:2u),"submission count");
        need(!adreno_ring_cmd(1,query,status,sizeof(status)),"profile query");
        klog("geom-bench perf kind=%u %ux%u tris=%u batch=%d n=%u ns=%llu submits=%u %s\n",
             kind,w,h,count,batch,repeats,(unsigned long long)(elapsed/repeats),adreno_dev()->fence-fence,status);
    }
    need(!adreno_ring_cmd(3,disable,status,sizeof(status)),"profile disable");
}
static void states(void) {
    /* Overlapping triangles exercise depth test/write, culling and ordered
     * additive/source-over paths. Both submits must retain draw ordering. */
    for(unsigned mode=0;mode<3;mode++) for(unsigned write=0;write<2;write++)
        for(unsigned cull=0;cull<4;cull++) {
            scene(81,59,2,1,5);
            memcpy(pos+9,pos,9*sizeof(float));
            for(unsigned v=0;v<3;v++) pos[9+v*3+2]-=0.25f;
            puti(meta.blend,mode); puti(meta.depth_write,write); puti(meta.cull,cull);
            clear(81*59); adreno_3d_geom_batch(0); reference_draw(81,59);
            memcpy(saved_fb,fb,81*59*4); memcpy(saved_z,depth,81*59*4);
            clear(81*59); adreno_3d_geom_batch(1); draw(81,59); guards(81*59);
            need(!memcmp(saved_fb,fb,81*59*4) && !memcmp(saved_z,depth,81*59*4),"render states");
            if(cull==3) for(unsigned i=0;i<81*59;i++) need(fb[i]==0xff102030u,"cull all");
            if(!write) for(unsigned i=0;i<81*59;i++) need(depth[i]==0xffffffffu,"depth mask");
            checked+=81*59; cases++;
        }
}
int main(int argc,char **argv) {
    const char *dir=argc>1?argv[1]:"/gpu-bench";
    setenv("ADRENO_ENABLE","1"); klog("geom-bench begin\n");
    need(adreno_2d_ready() && adreno_3d_ready(),"GPU init");
    alloc((void**)&fb,&fb_pa,(MAX_PIXELS+64)*4); alloc((void**)&depth,&z_pa,(MAX_PIXELS+64)*4);
    alloc((void**)&pos,&pos_pa,MAX_VERTS*6*4); alloc((void**)&col,&col_pa,MAX_VERTS*3*4);
    alloc((void**)&texture,&tex_pa,16*16*4);
    saved_fb=malloc(MAX_PIXELS*4); saved_z=malloc(MAX_PIXELS*4); need(saved_fb && saved_z,"snapshots");
    for(unsigned i=0;i<256;i++) texture[i]=0xff307090u+i*0x00010101u;
    const unsigned sizes[][2]={{1,1},{17,9},{64,64},{81,59},{257,131},{640,480},{1920,1080}};
    for(unsigned variant=0;variant<4;variant++) {
        unsigned kind=(variant&1)+1;
        const char *names[]={"color","texture","color-cached","texture-cached"};
        const char *name=names[variant];
        struct geom_meta reference_meta;
        if(variant>=2) {
            load(dir,kind==1?"color":"texture"); reference_program=program;
            reference_meta=meta;
        }
        load(dir,name); klog("geom-bench variant=%s\n",name);
        if(variant>=2) need(!memcmp(&reference_meta,&meta,sizeof(meta)),"reference reflection differs");
        for(unsigned s=0;s<7;s++) for(unsigned t=0;t<3;t++)
            correctness(sizes[s][0],sizes[s][1],t==0?1:t==1?64:513,kind,s+t);
        correctness(64,64,6144,kind,9);
        /* GPU first-frame clears, including RGBA -> ARGB conversion. */
        scene(64,64,1,kind,7); geom.first_frame=1; geom.clear_rgba=0xff302010u;
        clear(4096); adreno_3d_geom_batch(1); draw(64,64); guards(4096); oracle(64,64,kind,7);
        uint32_t fence=adreno_dev()->fence;
        geom.nverts=2; need(adreno_3d_draw(fb_pa,64,64,256,program,&geom,NULL,0,uniforms,meta.uniforms)<0,"invalid geometry");
        need(adreno_dev()->fence==fence,"decline submitted");
        if(kind==1) states();
        perf(64,64,1,kind); perf(640,480,64,kind); perf(1920,1080,513,kind);
        need(adreno_3d_program_free(program)==0,"program free");
        if(reference_program>=0) {
            need(adreno_3d_program_free(reference_program)==0,"reference free"); reference_program=-1;
        }
    }
    load(dir,"large"); scene(64,64,1,1,7); clear(4096); adreno_3d_geom_batch(1);
    uint32_t fence=adreno_dev()->fence; draw(64,64); guards(4096); oracle(64,64,1,7);
    need(adreno_dev()->fence-fence==2,"large IB fallback");
    need(adreno_3d_program_free(program)==0,"large free");
    load(dir,"invalid"); scene(64,64,1,1,7); clear(4096); geom.first_frame=1;
    fence=adreno_dev()->fence;
    need(adreno_3d_draw(fb_pa,64,64,256,program,&geom,NULL,0,uniforms,meta.uniforms)==ADRENO_E_DECLINED,"invalid raster");
    need(adreno_dev()->fence==fence,"invalid raster submitted");
    for(unsigned i=0;i<4096;i++) need(fb[i]==0xff102030u && depth[i]==0xffffffffu,"decline mutated canvas");
    need(adreno_3d_program_free(program)==0,"invalid free");
    klog("geom-bench fallback=passed preflight=passed\n");
    /* Releasing/reloading descriptors must remain safe after batched use. */
    for(unsigned i=0;i<32;i++) {
        load(dir,i&1?"texture":"color"); correctness(81,59,1,(i&1)?2:1,i);
        need(adreno_3d_program_free(program)==0,"reload free");
    }
    load(dir,"color"); unsigned frames=0;
    uint64_t begin=ns();
    while(ns()-begin<60000000000ull) {
        unsigned w=(frames&1)?640:257,h=(frames&1)?480:131;
        correctness(w,h,(frames%3)?64:513,1,frames);
        frames++;
    }
    need(adreno_3d_program_free(program)==0,"stress free");
    klog("geom-bench stress seconds=60 frames=%u\n",frames);
    adreno_3d_geom_batch(1);
    klog("geom-bench DONE cases=%u pixels=%llu fence=%u wedged=%d\n",cases,(unsigned long long)checked,adreno_dev()->fence,adreno_wedged());
    park();
}
